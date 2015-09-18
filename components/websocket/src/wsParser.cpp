#include <algorithm>
#include <string>

#include "wsConfig.h"
#include "wsParser.h"
#include "macroFuncs.h"
#include "sysHelper.h"

namespace parrot
{
    WsParser::WsParser(CallbackFunc cb, std::vector<char> &recvVec, 
                       const std::string &remoteIp, bool needMask,
                       const WsConfig &cfg):
        _recvVec(recvVec),
        _remoteIp(remoteIp),
        _needMask(needMask),
        _state(eParseState::Begin),
        _fin(false),
        _masked(false),
        _opCode(eOpCode::Continue),
        _fragmentDataType(eOpCode::Binary),
        _lastParseIt(),
        _pktBeginIt(),
        _callbackFunc(cb),
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

    void WsParser::parseHeader()
    {
        if (_payloadLen == 126)
        {            
            _payloadLen = uniNtohs(
                *(uint16_t*)&(*_lastParseIt));
            _lastParseIt += 2;
        }
        else if (_payloadLen == 127)
        {
            _payloadLen = uniNtohll(
                *(uint64_t*)&(*_lastParseIt));
            _lastParseIt += 8;
        }

        if (_masked)
        {
            std::copy_n(_lastParseIt, 4, _maskingKey.begin());
            _lastParseIt += 4;
        }
    }

    void WsParser::parseBody()
    {
        auto pktEnd = _lastParseIt + _payloadLen;
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
            _callbackFunc(_opCode, _lastParseIt, pktEnd);
        }
        else if (_fin)
        {
            // Fragmented frame, save to _packetVec.
            if (_opCode == eOpCode::Continue)
            {
                // save.
                std::copy_n(_lastParseIt, _payloadLen, 
                            std::back_inserter(_packetVec));

                _callbackFunc(_opCode, _packetVec.begin(), _packetVec.end());

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
                _callbackFunc(_opCode, _lastParseIt, pktEnd);
            }
        }
        else
        {
            // save.
            std::copy_n(_lastParseIt, _payloadLen, 
                        std::back_inserter(_packetVec));
        }

        _lastParseIt += _payloadLen;
    }

    eCodes WsParser::doParse()
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
                    // The 2nd, 3rd & 4th bits must be ZERO.
                    _parseResult = eCodes::WS_ProtocolError;
                    return eCodes::ST_Ok;
                }

                // Get opcode.
                _opCode = (eOpCode)(firstByte & 0x0F);
                if ((_opCode >  eOpCode::Binary && _opCode < eOpCode::Close) ||
                    (_opCode >  eOpCode::Pong))
                {
                    // Peer should not use reserved opcode.
                    _parseResult = eCodes::WS_ProtocolError;
                    return eCodes::ST_Ok;
                }

                if (_opCode == eOpCode::Text)
                {
                    // We don't support text payload.
                    _parseResult = eCodes::WS_UnsupportedData;
                    return eCodes::ST_Ok;
                }

                // Fragmented data should save data type.
                if (!_fin && _opCode != eOpCode::Continue)
                {
                    // First fragmented data frame.
                    if (_opCode != eOpCode::Binary)
                    {
                        _parseResult = eCodes::WS_ProtocolError;
                        return eCodes::ST_Ok;
                    }
                    _packetVec.reserve(_recvVec.capacity() * 4);
                    _fragmentDataType = _opCode;
                }

                uint8_t secondByte = (uint8_t)*_lastParseIt++;
                _masked = secondByte & ((uint8_t)1 << 8);
                _payloadLen = secondByte & (uint8_t)127;

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

                if (_payloadLen + _headerLen > _recvVec.capacity())
                {
                    if (_payloadLen + _headerLen > _config._maxPacketLen)
                    {
                        _parseResult = eCodes::WS_MessageTooBig;
                        return eCodes::ST_Ok;
                    }
                    
                    uint32_t times = (_payloadLen + _headerLen) / 
                        _recvVec.capacity() + 1;
                    _recvVec.reserve(times * _recvVec.capacity());
                }
                
                if (_recvVec.end() - _pktBeginIt < _headerLen)
                {
                    _state = eParseState::ParsingHeader;
                    return eCodes::ST_NeedRecv;
                }

                parseHeader();

                _state = eParseState::ParsingBody;
                return doParse();
            }
            break;

            case eParseState::ParsingHeader:
            {
                if (_recvVec.end() - _pktBeginIt < _headerLen)
                {
                    return eCodes::ST_NeedRecv;
                }

                parseHeader();
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

                parseBody();
                _state = eParseState::Begin;
                return eCodes::ST_Ok;
            }
            break;
        }

        PARROT_ASSERT(false);
        return eCodes::ERR_Fail;
    }

    eCodes WsParser::parse()
    {
        eCodes code;
        while (true)
        {
            _pktBeginIt = _pktBeginIt + _headerLen + _payloadLen;

            code = doParse();

            if (code == eCodes::ST_Ok)
            {
                if (_parseResult != eCodes::ST_Ok)
                {
                    return eCodes::ST_Complete;
                }
                else
                {
                    continue;
                }
            }
            else if (code == eCodes::ST_NeedRecv)
            {
                uint32_t rcvdLen = _recvVec.end() - _pktBeginIt;
                std::move(_pktBeginIt, _recvVec.end(), _recvVec.begin());
                _recvVec.resize(rcvdLen);
            }

            return code;
        }
    }
}
