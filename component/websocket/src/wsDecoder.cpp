#include <algorithm>
#include <string>
#include <iostream>

#include "logger.h"
#include "json.h"
#include "wsConfig.h"
#include "wsPacket.h"
#include "wsDecoder.h"
#include "macroFuncs.h"
#include "sysHelper.h"
#include "wsDefinition.h"
#include "ioEvent.h"
#include "wsTranslayer.h"

namespace parrot
{
WsDecoder::WsDecoder(WsTranslayer& trans)
    : _recvVec(trans._recvVec),
      _rcvdLen(trans._rcvdLen),
      _remoteIp(trans._io.getRemoteAddr()),
      _needMask(trans._needRecvMasked),
      _callbackFunc(trans._onPacketCb),
      _config(trans._config),
      _state(eParseState::Begin),
      _fin(false),
      _masked(false),
      _opCode(eOpCode::Continue),
      _fragmentDataType(eOpCode::Binary),
      _lastParsePos(0),
      _pktBeginPos(0),
      _headerLen(0),
      _payloadLen(0),
      _maskingKey(),
      _parseResult(eCodes::ST_Ok),
      _packetVec()
{
    _recvVec.clear();
}

std::unique_ptr<WsPacket>
WsDecoder::createWsPacket(const std::vector<char>::iterator& begin,
                          const std::vector<char>::iterator& end)
{
    std::vector<char> payload;
    payload.reserve(end - begin);
    std::copy(begin, end, std::back_inserter(payload));

    std::cout << std::string(&payload[0], payload.size()) << std::endl;
    
    std::unique_ptr<WsPacket> pkt(new WsPacket());
    pkt->setPacket(_opCode, std::move(payload));

    std::cout << "createWsPacket" << std::endl;
    return pkt;
}

void WsDecoder::parseHeader()
{
    if (_payloadLen == 126)
    {
        _payloadLen = uniNtohs(*(uint16_t*)&(_recvVec[_lastParsePos]));
        _lastParsePos += 2;
    }
    else if (_payloadLen == 127)
    {
        _payloadLen = uniNtohll(*(uint64_t*)&(_recvVec[_lastParsePos]));
        _lastParsePos += 8;
    }

    if (_masked)
    {
        // The mask key is not a integer. However, We can use a 32 bit
        // integer to save the mask key without bother the ntohxx functions.
        _maskingKey = *reinterpret_cast<uint32_t*>(&(_recvVec[_lastParsePos]));
        _lastParsePos += 4;
    }
}

void WsDecoder::unmaskData()
{
    // Unmasking data.
    auto* p        = reinterpret_cast<uint32_t*>(&_recvVec[_lastParsePos]);
    uint64_t len32 = _payloadLen / 4;
    uint64_t idx   = 0;

    for (; idx < len32; ++idx)
    {
        *(p + idx) ^= _maskingKey;
    }

    switch (_payloadLen % 4)
    {
        case 3:
        {
            _recvVec[_lastParsePos + _payloadLen - 1] ^=
                (reinterpret_cast<uint8_t*>(&_maskingKey))[2];
        }
        // No break.

        case 2:
        {
            _recvVec[_lastParsePos + _payloadLen - 2] ^=
                (reinterpret_cast<uint8_t*>(&_maskingKey))[1];
        }
        // No break.

        case 1:
        {
            _recvVec[_lastParsePos + _payloadLen - 3] ^=
                (reinterpret_cast<uint8_t*>(&_maskingKey))[0];
        }
        // No break.

        case 0:
        {
            // Do nothing.
        }
    }
}

void WsDecoder::parseBody()
{
    if (_masked)
    {
        unmaskData();
    }

    if (_fin && _packetVec.size() == 0)
    {
        // Not fragmented.
        auto begin = _recvVec.begin() + _lastParsePos;
        auto pkt = createWsPacket(begin, begin + _payloadLen);
        _callbackFunc(std::move(pkt));
    }
    else if (_fin)
    {
        // Fragmented frame, save to _packetVec.
        if (_opCode == eOpCode::Continue)
        {
            // save.
            std::copy_n(&_recvVec[_lastParsePos], _payloadLen,
                        std::back_inserter(_packetVec));

            auto pkt = createWsPacket(_packetVec.begin(), _packetVec.end());
            _callbackFunc(std::move(pkt));

            // Reset packet vec.
            _packetVec.clear();
            _packetVec.shrink_to_fit();
        }
        else if (_opCode == eOpCode::Binary)
        {
            // _packetVec has data. opcode must not be binary.
            _parseResult = eCodes::WS_ProtocolError;
            return;
        }
        else
        {
            // Control frame.
            auto begin = _recvVec.begin() + _lastParsePos;
            auto pkt = createWsPacket(begin, begin + _payloadLen);
            _callbackFunc(std::move(pkt));
        }
    }
    else
    {
        // Fragmented packet. Save to _packetVec.
        std::copy_n(&_recvVec[_lastParsePos], _payloadLen,
                    std::back_inserter(_packetVec));
    }

    _lastParsePos += _payloadLen;
}

eCodes WsDecoder::doParse()
{
    _parseResult = eCodes::ST_Ok;

    switch (_state)
    {
        case eParseState::Begin:
        {
            if (_rcvdLen - _pktBeginPos < 2)
            {
                return eCodes::ST_NeedRecv;
            }

            uint8_t firstByte = static_cast<uint8_t>(_recvVec[_lastParsePos++]);
            _fin = firstByte & ((uint8_t)1 << 7);
            if ((firstByte & ((uint8_t)1 << 6)) != 0 ||
                (firstByte & ((uint8_t)1 << 5)) != 0 ||
                (firstByte & ((uint8_t)1 << 4)) != 0)
            {
                LOG_WARN("WsDecoder::doParse: Protocol error. "
                         "Remote is "
                         << _remoteIp << ".");
                // The 2nd, 3rd & 4th bits must be ZERO.
                _parseResult = eCodes::WS_ProtocolError;
                return eCodes::ST_Ok;
            }

            // Get opcode.
            _opCode = (eOpCode)(firstByte & 0x0F);
            if ((_opCode > eOpCode::Binary && _opCode < eOpCode::Close) ||
                (_opCode > eOpCode::Pong))
            {
                LOG_WARN("WsDecoder::doParse: Protocol error. "
                         "Remote is "
                         << _remoteIp << ".");
                // Peer should not use reserved opcode.
                _parseResult = eCodes::WS_ProtocolError;
                return eCodes::ST_Ok;
            }

            if (_opCode == eOpCode::Text)
            {
                // We don't support text payload.
                LOG_WARN("WsDecoder::doParse: Only binary is accepted. "
                         "Remote is "
                         << _remoteIp << ".");
                _parseResult = eCodes::WS_InvalidFramePayloadData;
                return eCodes::ST_Ok;
            }

            // Fragmented data should save data type. The fin flag of
            // control frame must be 1. Only data frame can be 0.
            if (!_fin && _opCode != eOpCode::Continue)
            {
                // First fragmented data frame.
                if (_opCode != eOpCode::Binary)
                {
                    LOG_WARN("WsDecoder::doParse: Only binary is accepted. "
                             "Remote is "
                             << _remoteIp << ".");
                    _parseResult = eCodes::WS_InvalidFramePayloadData;
                    return eCodes::ST_Ok;
                }
                _packetVec.reserve(_recvVec.capacity() * 4);
                _fragmentDataType = _opCode;
            }

            uint8_t secondByte = static_cast<uint8_t>(_recvVec[_lastParsePos++]);
            _masked            = secondByte & 0x80;
            _payloadLen        = secondByte & 0x7F;

            if ((_needMask && !_masked) || (!_needMask && _masked))
            {
                _parseResult = eCodes::WS_InvalidFramePayloadData;
                return eCodes::ST_Ok;
            }

            if (_payloadLen == 126)
            {
                _headerLen = _masked ? (2 + 2 + 4) : (2 + 2);
            }
            else if (_payloadLen == 127)
            {
                _headerLen = _masked ? (2 + 8 + 4) : (2 + 8);
            }
            else
            {
                _headerLen = _masked ? (2 + 4) : 2;
            }

            // Here we got a big packet which length is greater the the
            // capacity of the _recvVec. I'll set the capacity according
            // to the packet length.
            if (_payloadLen + _headerLen > _recvVec.capacity())
            {
                if (_payloadLen + _headerLen > _config._maxPacketLen)
                {
                    // Way too big. Is peer a attacker?
                    _parseResult = eCodes::WS_MessageTooBig;
                    return eCodes::ST_Ok;
                }

                _recvVec.reserve(_payloadLen + _headerLen);
            }

            // Pre analyzing completed, goto next step.
            _state = eParseState::ParsingHeader;
        }
        // No break;

        case eParseState::ParsingHeader:
        {
            if (_rcvdLen - _pktBeginPos < _headerLen)
            {
                return eCodes::ST_NeedRecv;
            }

            // Here we have all header data. Just parse it.
            parseHeader();

            // Next we need to parse body.
            _state = eParseState::ParsingBody;
        }
        // No break;

        case eParseState::ParsingBody:
        {
            if (_rcvdLen - _lastParsePos < _payloadLen)
            {
                return eCodes::ST_NeedRecv;
            }

            // Here we have all body data. Now parse body.
            parseBody();

            // Next, we reset the state machine. And return OK to notify
            // up layer that the parsing has been completed.
            _state = eParseState::Begin;
            return eCodes::ST_Ok;
        }
        break;

        default:
        {
        }
        break;
    }

    PARROT_ASSERT(false);
    return eCodes::ERR_Fail;
}

eCodes WsDecoder::parse()
{
    eCodes code;
    bool needMoveBuff = false;

    while (true)
    {
        code = doParse();

        if (code == eCodes::ST_Ok)
        {
            if (_parseResult != eCodes::ST_Ok)
            {
                return eCodes::ST_Complete;
            }
            else
            {
                if (_lastParsePos != _rcvdLen)
                {
                    // Received more than one packet. Continue parsing.
                    _pktBeginPos = _lastParsePos;
                    needMoveBuff = true;
                    continue;
                }

                // Here we check the receiving vector capacity, if larger
                // than the configuration, set it back to save memory.
                if (_recvVec.capacity() > _config._recvBuffLen)
                {
                    // Shrink buffer.
                    if (++_largePktCount % 10 == 0) // Warning every 10 times.
                    {
                        LOG_WARN("WsDecoder::parse: Received "
                                 << _largePktCount
                                 << " large packets. "
                                    "Consider set bigger receive buffer.");
                    }
                    _recvVec.resize(_config._recvBuffLen);
                    _recvVec.shrink_to_fit();
                }

                _rcvdLen      = 0;
                _pktBeginPos  = 0;
                _lastParsePos = 0;
                return eCodes::ST_Complete;
            }
        }
        else if (code == eCodes::ST_NeedRecv)
        {
            if (needMoveBuff)
            {
                uint64_t rcvdLen = _rcvdLen - _lastParsePos;
                std::move(&_recvVec[_lastParsePos], &_recvVec[_rcvdLen],
                          _recvVec.begin());
                _rcvdLen      = rcvdLen;
                _lastParsePos = _lastParsePos - _pktBeginPos;
                _pktBeginPos  = 0;
            }
        }
        return code;
    }
}
}
