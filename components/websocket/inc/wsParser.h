#ifndef __COMPONENT_WEBSOCKET_INC_WSPARSER_H__
#define __COMPONENT_WEBSOCKET_INC_WSPARSER_H__

#include <unordered_map>

#include "httpParser.h"
#include "codes.h"

namespace parrot
{
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
        WsParser(WsTranslayer &trans, bool needMask);
        virtual ~WsParser() = default;
        WsParser(const WsParser &) = delete;
        WsParser& operator=(const WsParser &) = delete;

      public:
        bool isFin() const { return _fin; }
        bool isMasked() const { return _masked; }
        eCodes getParseResult() const { return _parseResult; }
        eCodes parse();

      protected:
        virtual void parseBody();

      private:
        eCodes doParse();
        void parseHeader();

      private:
        WsTranslayer &                           _trans;
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
