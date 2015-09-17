#ifndef __COMPONENT_WEBSOCKET_INC_WSPARSER_H__
#define __COMPONENT_WEBSOCKET_INC_WSPARSER_H__

#include <vector>
#include <string>
#include "codes.h"

namespace parrot
{
    // WsParser is the websocket parser for both server side and 
    // client side. RPC packet parser will also use this class.
    template<typename WsIo>
        class WsParser
    {
      public:
        enum class eOpCode
        {
            Continue   = 0x0,
            Text       = 0x1,
            Binary     = 0x2,
            // 0x3-7 are reserved. 
            Close      = 0x8,
            Ping       = 0x9,
            Pong       = 0xA
            // 0xB-F are reserved. 
         };

        enum eParseState
        {
            Begin,
            ParsingHeader,
            ParsingBody
        };

      public:
        WsParser(WsIo &io,
                 std::vector<char> &recvVec, 
                 const std::string &remoteIp,
                 bool needMask)
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
            _headerLen(0),
            _payloadLen(0),
            _maskingKey(),
            _parseResult(eCodes::ST_Ok),
            _packetVec()
            {
                _recvVec.clear();
                _lastParsePos = _recvVec.begin();
                _pktBeginIt = _lastParsePos;
            }

        ~WsParser() = default;

        WsParser(const WsParser &) = delete;
        WsParser& operator=(const WsParser &) = delete;

      public:
        // getResult
        //
        // Get the parsing result.
        //
        // Return
        //  * WS_UnsupportedData
        //  * WS_ProtocolError
        //  * WS_MessageTooBig        
        //  * ST_Ok
        eCodes getResult() const { return _parseResult; }

        // parse
        // 
        // Parse the buffer, if received more than one packets,
        // it will continue to parse. E.g. If we received 2 packets
        // and only parts of 3rd packet, it will parse the first two
        // packet, then, move the bytes of 3rd packet to the begin 
        // of the buffer, and reset the recvVec size to the received
        // length of 3rd packet.
        //
        // return
        //  * ST_Complete     Parsing completed.
        //  * ST_NeedRecv     Need receive more data to parse.
        eCodes parse()
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
                    uint32_t rcvdLen = _recvVec.end() - _pktBeginIt;
                    std::move(_pktBeginIt, _recvVec.end(), 
                              _recvVec.begin());
                    _recvVec.resize(rcvdLen);
                }

                return code;
            }
        }


      private:
        // isFin
        //
        // Fin is the flag used to determine whether the whole
        // packet is received. If a big packet is fragmented to
        // several packets, the fragmented packets will be sent
        // in sequence. And the last fragmented packet's fin 
        // flag must be set to 1. Other's fin must be set to 0.
        // If the packet is not fragmented, the fin flag must
        // be 1.
        //
        // return:
        //  If fin flag is 1, returns true.
        bool isFin() const { return _fin; }

        // doParse
        //
        // This function parses the buffer and invokes the 
        // callback function if parsing successfully completed.
        // The client must invoke the 'getResult' to retrieve
        // the parsing result.
        //
        // return:
        //  * ST_NeedRecv    // Hasn't receive the full packet, 
        //                        need recevie more.
        //  * ST_Ok          // Finished parsing.
        eCodes doParse()
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
                    _opCode = (eOpcode)firstByte & 0x0F;
                    if ((_opCode >  eOpCode::Binary && 
                         _opCode < eOpCode::Close) || 
                        (_opCode >  eOpCode::Pone))
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
                        _packetVec.reserve(
                            WsTranslayer<WsIo>::kRecvBuffLen * 4);
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
                        if (_payloadLen + _headerLen > 
                            WsTranslayer<WsIo>::kRecvMaxLen)
                        {
                            _parseResult = eCodes::MessageTooBig;
                            return eCodes::ST_Ok;
                        }
                    
                        uint32_t times = (_payloadLen + _headerLen) / 
                            WsTranslayer<WsIo>::kRecvBuffLen + 1;
                        _recvVec.reserve(
                            times * WsTranslayer<WsIo>::kRecvBuffLen);
                    }
                
                    if (_recvVec.end() - _pktBeginIt < _headerLen)
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
                    if (_recvVec.end() - _lastParseIt < _payloadLen)
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


        // parseHeader
        //
        // Parses the websocket header. Get the length of payload
        // and mask key if exists.
        void parseHeader()
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


        // parseBody
        //
        // Unmasking the payload. If the packet is fragmented, and
        // hasn't receive all, saves it. If received all or not
        // fragmented, invoke the callback to notify uplayer.
        void parseBody()
        {
            auto pktEnd = _lastParseIt + _payloadLen;
            if (_masked)
            {
                // Unmasking data.
                auto i = 0u;
                for (auto it = _lastParseIt; it != end; ++it)
                {
                    *it = *it ^ _maskingKey[i++ % 4];
                }
            }

            if (_fin && _packetVec.size() == 0)
            {
                // Not fragmented.
                _io.onPayload(_opCode, _lastParseIt, pktEnd);
            }
            else if (_fin)
            {
                // Fragmented frame, save to _packetVec.
                if (_opCode == eOpCode::Continue)
                {
                    // save.
                    std::copy_n(_lastParsePos, _payloadLen, 
                                std::back_inserter(_packetVec));

                    _io.onPayload(_opCode, _packetVec.begin(), 
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
                    _io.onPayload(_opCode, _lastParseIt, pktEnd);
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


      private:
        WsIo &                                   _io;
        std::vector<char>&                       _recvVec;
        const std::string &                      _remoteIp;
        bool                                     _needMask;
        eParseState                              _state;
        bool                                     _fin;
        bool                                     _masked;
        eOpCode                                  _opCode;
        eOpCode                                  _fragmentDataType;
        std::vector<char>::iterator              _lastParseIt;
        std::vector<char>::iterator              _pktBeginIt;
        uint8_t                                  _headerLen;
        uint64_t                                 _payloadLen;
        std::array<char, 4>                      _maskingKey;
        Codes                                    _parseResult;
        std::vector<char>                        _packetVec;
    };
}
#endif
