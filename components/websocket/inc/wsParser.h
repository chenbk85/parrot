#ifndef __COMPONENT_WEBSOCKET_INC_WSPARSER_H__
#define __COMPONENT_WEBSOCKET_INC_WSPARSER_H__

#include <unordered_map>

#include "httpParser.h"
#include "codes.h"

namespace parrot
{
    class WsTranslayer;

    class WsParser
    {
        using HeaderDic = std::unordered_map<std::string, std::string>;

        enum
        {
            kWebSockerVer = 16
        };

        enum class WsParseState
        {
            HttpHandshake,
            DataFrame
        };

      public:
        explicit WsParser(WsTranslayer *trans);
        ~WsParser();
        WsParser(const WsParser &) = delete;
        WsParser& operator=(const WsParser &) = delete;

      public:
        Codes parse();

      private:
        WsParseState                             _parseState;
        WsTranslayer *                           _trans;
        HeaderDic                                _headerDic;
        string                                   _lastHeaderField;
    };
}
#endif
