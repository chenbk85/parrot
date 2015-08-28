#ifndef __COMPONENT_WEBSOCKET_INC_WSPARSER_H__
#define __COMPONENT_WEBSOCKET_INC_WSPARSER_H__

#include <unordered_map>

#include "codes.h"


namespace parrot
{
    class WsTranslayer;

    class WsParser
    {
        using HeaderDic = std::unordered_map<std::string, std::string>;

      public:
        explicit WsParser(WsTranslayer *trans);
        ~WsParser();
        WsParser(const WsParser &) = delete;
        WsParser& operator=(const WsParser &) = delete;

      public:
        Codes parse();

      private:
        WsTranslayer *                         _trans;
        HeaderDic                              _headerDic;
    };
}

#endif
