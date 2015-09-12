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
            RecevingHeader,
            RecevingBody
        };

      public:
        explicit WsParser(WsTranslayer *trans);
        ~WsParser();
        WsParser(const WsParser &) = delete;
        WsParser& operator=(const WsParser &) = delete;

      public:
        Codes parse();

      private:
        eParseState                              _state;
        bool                                     _fin;
        bool                                     _masked;
        eOpCode                                  _opCode;
        uint32_t                                 _bodyLen;
        uint32_t                                 _pktBeginPos;
        uint32_t                                 _lastParsePos;
        uint8_t                                  _headerLen;
        uint64_t                                 _payloadLen;
        Codes                                    _parseResult;
        std::vector<char>                        _packetVec;
    };
}
#endif
