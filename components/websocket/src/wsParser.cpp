#include "wsTranslayer.h"
#include "wsParser.h"
#include "macroFuncs.h"
#include "stringHelper.h" // strToLower() ...

namespace parrot
{
    WsParser::WsParser(WsTranslayer trans):
        _parseState(HttpHandshake),
        _trans(trans),
        _headerDic()
    {
    }

    WsParser::~WsParser()
    {
        _trans = nullptr;
    }

    Codes WsParser::parseHandshake()
    {
    }

    Codes WsParser::parse()
    {
        switch (_wsParseStatus)
        {
            
        }
    }

    Codes WsParser::parseHttpHandshake()
    {
        // Init settings.
        http_parser_settings settings;
        settings.on_url = onUrl;
        settings.on_status = onStatus;
        settings.on_header_field = onHeaderField;
        settings.on_header_value = onHeaderValue;
        settings.on_headers_complete = onHeaderComplete;
        settings.on_body = onBody;
        settings.on_message_complete = onMsgComplete;

        // Init parser.
        _parser = new http_parser();
        if (!_parser)
        {
            PARROT_ASSERT(0);
        }
        http_parser_init(parser, HTTP_REQUEST);
        auto ret = http_parser_execute(_parser, &settings,
                                       &(_trans->recvVec)[0],
                                       (_trans->recvBuff).size());
        
    }

    int WsParser::onUrl(http_parser *p, const char *at, size_t length)
    {
        if (_headerDic.count("url") != 0 ||
            p->method != static_cast<int>(HTTP_GET))
        {
            return 1;
        }
        _headerDic.emplace(std::make_pair(std::string("url"),
                                          std::string(at, length)));
        return 0;
    }

    int WsParser::onHeaderField(http_parser *p, const char *at, size_t length)
    {
        _lastHeaderField = std::string(at, length);
        strToLower(_lastHeaderField);
        
        if (_headerDic.count(_lastHeaderField) != 0)
        {
            return 1;
        }
        _headerDic.emplace(std::make_pair(_lastHeaderField, ""));
        return 0;
    }

    int WsParser::onHeaderValue(http_parser *p, const char *at, size_t length)
    {
        if (_lastHeaderField.empty())
        {
            return 1;
        }

        std::string value = std::string(at, length);
        strToLower(value);
        
        _headerDic.emplace(std::make_pair(_lastHeaderField,
                                          std::string(at, length)));
        _lastHeaderField = "";
        return 0;
    }

    int WsParser::onHeaderComplete(http_parser *p)
    {

        
        delete _parser;
        _parser = nullptr;
    }
}
