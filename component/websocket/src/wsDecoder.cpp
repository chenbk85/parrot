#include <algorithm>
#include <string>

#include "logger.h"
#include "json.h"
#include "wsConfig.h"
#include "wsPacket.h"
#include "wsDecoder.h"
#include "macroFuncs.h"
#include "sysHelper.h"
#include "wsDefinition.h"

namespace parrot
{
WsDecoder::WsDecoder(CallbackFunc &&cb,
                   std::vector<char>& recvVec,
                   const std::string& remoteIp,
                   bool needMask,
                   const WsConfig& cfg)
    : _recvVec(recvVec),
      _remoteIp(remoteIp),
      _needMask(needMask),
      _state(eParseState::Begin),
      _fin(false),
      _masked(false),
      _opCode(eOpCode::Continue),
      _fragmentDataType(eOpCode::Binary),
      _lastParseIt(),
      _pktBeginIt(),
      _callbackFunc(std::move(cb)),
      _headerLen(0),
      _payloadLen(0),
      _maskingKey(),
      _parseResult(eCodes::ST_Ok),
      _packetVec(),
      _config(cfg)
{
    _recvVec.clear();
    _lastParseIt = _recvVec.begin();
    _pktBeginIt = _lastParseIt;
}

std::unique_ptr<WsPacket>
WsDecoder::createWsPacket(const std::vector<char>::iterator& begin,
                         const std::vector<char>::iterator& end)
{
    std::vector<char> payload(end - begin);
    std::copy(begin, end, payload.begin());

    std::unique_ptr<WsPacket> pkt(new WsPacket());
    pkt->setPacket(_opCode, std::move(payload));
    return pkt;
}

void WsDecoder::parseHeader()
{
    if (_payloadLen == 126)
    {
        _payloadLen = uniNtohs(*(uint16_t*)&(*_lastParseIt));
        _lastParseIt += 2;
    }
    else if (_payloadLen == 127)
    {
        _payloadLen = uniNtohll(*(uint64_t*)&(*_lastParseIt));
        _lastParseIt += 8;
    }

    if (_masked)
    {
        std::copy_n(_lastParseIt, 4, _maskingKey.begin());
        _lastParseIt += 4;
    }
}

void WsDecoder::parseBody()
{
    auto pktEnd = _pktBeginIt + _headerLen + _payloadLen;
    if (_masked)
    {
        // Unmasking data.
        auto i = 0u;
        for (auto it = _lastParseIt; it != pktEnd; ++it)
        {
            *it = *it ^ _maskingKey[i++ % 4];
        }
    }

    if (_fin && _packetVec.size() == 0)
    {
        // Not fragmented.
        auto pkt = createWsPacket(_lastParseIt, pktEnd);
        _callbackFunc(std::move(pkt));
    }
    else if (_fin)
    {
        // Fragmented frame, save to _packetVec.
        if (_opCode == eOpCode::Continue)
        {
            // save.
            std::copy_n(_lastParseIt, _payloadLen,
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
            auto pkt = createWsPacket(_lastParseIt, pktEnd);
            _callbackFunc(std::move(pkt));
        }
    }
    else
    {
        // Fragmented packet. Save to _packetVec.
        std::copy_n(_lastParseIt, _payloadLen, std::back_inserter(_packetVec));
    }

    _lastParseIt += _payloadLen;
}

eCodes WsDecoder::doParse()
{
    _parseResult = eCodes::ST_Ok;

    switch (_state)
    {
        case eParseState::Begin:
        {
            if (_recvVec.end() - _pktBeginIt < 2)
            {
                return eCodes::ST_NeedRecv;
            }
            break;

            uint8_t firstByte = (uint8_t)*_lastParseIt++;
            _fin = firstByte & ((uint8_t)1 << 8);
            if ((firstByte & ((uint8_t)1 << 7)) != 0 ||
                (firstByte & ((uint8_t)1 << 6)) != 0 ||
                (firstByte & ((uint8_t)1 << 5)) != 0)
            {
                LOG_WARN("WsDecoder::doParse: Protocol error. "
                         "Remote is " << _remoteIp << ".");                
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
                         "Remote is " << _remoteIp << ".");
                // Peer should not use reserved opcode.
                _parseResult = eCodes::WS_ProtocolError;
                return eCodes::ST_Ok;
            }

            if (_opCode == eOpCode::Text)
            {
                // We don't support text payload.
                LOG_WARN("WsDecoder::doParse: Only binary is accepted. "
                         "Remote is " << _remoteIp << ".");
                _parseResult = eCodes::WS_InvalidFramePayloadData;
                return eCodes::ST_Ok;
            }

            // Fragmented data should save data type.
            if (!_fin && _opCode != eOpCode::Continue)
            {
                // First fragmented data frame.
                if (_opCode != eOpCode::Binary)
                {
                    LOG_WARN("WsDecoder::doParse: Only binary is accepted. "
                             "Remote is " << _remoteIp << ".");
                    _parseResult = eCodes::WS_InvalidFramePayloadData;
                    return eCodes::ST_Ok;
                }
                _packetVec.reserve(_recvVec.capacity() * 4);
                _fragmentDataType = _opCode;
            }

            uint8_t secondByte = (uint8_t)*_lastParseIt++;
            _masked = secondByte & 0x80;
            _payloadLen = secondByte & 0x7F;

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
            }

            // Pre analyzing completed, goto next step.
            _state = eParseState::ParsingHeader;
            return doParse();
        }
        break;

        case eParseState::ParsingHeader:
        {
            if (_recvVec.end() - _pktBeginIt < _headerLen)
            {
                return eCodes::ST_NeedRecv;
            }

            // Here we have all header data. Just parse it.
            parseHeader();

            // Next we need to parse body.
            _state = eParseState::ParsingBody;
            return doParse();
        }
        break;

        case eParseState::ParsingBody:
        {
            if ((uint32_t)(_recvVec.end() - _lastParseIt) < _payloadLen)
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
                if (_lastParseIt != _recvVec.end())
                {
                    // Received more than one packet. Continue parsing.
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
                    _recvVec.clear();
                    _pktBeginIt = _recvVec.begin();
                }

                return eCodes::ST_Complete;
            }
        }
        else if (code == eCodes::ST_NeedRecv)
        {
            if (_lastParseIt != _recvVec.end())
            {
                uint32_t rcvdLen = _recvVec.end() - _lastParseIt;
                std::move(_lastParseIt, _recvVec.end(), _recvVec.begin());
                _recvVec.resize(rcvdLen);
                _pktBeginIt = _recvVec.begin();
            }
        }

        return code;
    }
}
}
