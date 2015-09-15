#include <string>

#include "wsTranslayer.h"
#include "wsParser.h"
#include "macroFuncs.h"

namespace parrot
{
    WsParser::WsParser(WsTranslayer &trans, bool needMask):
        _trans(trans),
        _needMask(needMask),
        _state(eParseState::Begin),
        _fin(false),
        _masked(false),
        _opCode(eOpCode::Continue),
        _fragmentDataType(eOpCode::Binary),
        _lastParseIt(_trans._recvVec.begin()),
        _pktBeginIt(_lastParseIt),
        _headerLen(0),
        _payloadLen(0),
        _maskingKey(),
        _parseResult(eCodes::ST_Ok),
        _packetVec()
    {
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
            std::copy_n(_lastParseIt, 4, std::back_inserter(_maskingKey));
            _lastParseIt += 4;
        }
    }

    void WsParser::parseBody()
    {
        auto pktEnd = _lastParseIt + _payloadLen;
        if (_masked)
        {
            auto i = 0u;
            for (auto it = _lastParseIt; it != end; ++it)
            {
                *it = *it ^ _maskingKey[i++ % 4];
            }
        }

        if (_fin && _packetVec.size() == 0)
        {
            // Not fragmented.
            _trans.io.onPayload(_opCode, _lastParseIt, pktEnd);
        }
        else if (_fin)
        {
            // Fragmented frame, save to _packetVec.
            if (_opCode == eOpCode::Continue)
            {
                // save.
                std::copy_n(_lastParsePos, _payloadLen, 
                            std::back_inserter(_packetVec));

                _trans.io.onPayload(_opCode, _packetVec.begin(), 
                                    _packetVec.end());

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
                _trans.io.onPayload(_opCode, _lastParseIt, pktEnd);
            }
        }
        else
        {
            // save.
            std::copy_n(_lastParsePos, _payloadLen, 
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
                if (_trans._recvVec.end() - _pktBeginIt < 2)
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
                _opCode = (eOpcode)firstByte & 0x0F;
                if ((_opCode >  eOpCode::Binary && _opCode < eOpCode::Close) ||
                    (_opCode >  eOpCode::Pone)  || 
                    (_opCode == eOpCode::Text))
                {
                    // Peer should not use reserved opcode.
                    _parseResult = eCodes::WS_ProtocolError;
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
                    _packetVec.reserve(WsTranslayer::kRecvBuffLen * 4);
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

                if (_payloadLen + _headerLen > _trans._recvVec.capacity())
                {
                    if (_payloadLen + _headerLen > WsTranslayer::kRecvMaxLen)
                    {
                        _parseResult = eCodes::MessageTooBig;
                        return eCodes::ST_Ok;
                    }
                    
                    uint32_t times = (_payloadLen + _headerLen) / 
                        WsTranslayer::kRecvBuffLen + 1;
                    _trans._recvVec.reserve(times * WsTranslayer::kRecvBuffLen);
                }
                
                if (_trans._recvVec.end() - _pktBeginIt < _headerLen)
                {
                    _state = eParseState::RecevingHeader;
                    return eCodes::ST_NeedRecv;
                }

                parseHeader();

                _state = eParseState::RecevingBody;
                return doParse();
            }
            break;

            case eParseState::ParsingHeader:
            {
                if (_trans._recvVec.end() - _pktBeginIt < _headerLen)
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
                if (_trans._recvVec.end() - _lastParseIt < _payloadLen)
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
                if (_parseResult != eST_Ok)
                {
                    return ST_Complete;
                }
                else
                {
                    continue;
                }
            }
            else if (code == eCodes::ST_NeedRecv)
            {
                uint32_t rcvdLen = _trans._recvVec.end() - _pktBeginIt;
                std::move(_pktBeginIt, _trans._recvVec.end(), 
                          _trans._recvVec.begin());
                _trans._recvVec.resize(rcvdLen);
            }

            return code;
        }
    }
}
