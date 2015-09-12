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
        explicit WsParser(WsTranslayer *trans);
        ~WsParser();
        WsParser(const WsParser &) = delete;
        WsParser& operator=(const WsParser &) = delete;

      public:
        void parse();

      private:
        WsTranslayer &                           _trans;
        uint32_t                                 _lastParsePos;
    };
}
#endif
