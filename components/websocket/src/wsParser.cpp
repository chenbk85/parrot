#include <string>

#include "wsTranslayer.h"
#include "wsParser.h"
#include "macroFuncs.h"
#include "stringHelper.h" // strToLower() ...

namespace parrot
{
    WsParser::WsParser(WsTranslayer trans):
        _parseState(HttpHandshake),
        _trans(trans),
        _headerDic(),
        _lastHeaderField(),
        _lastParsePos(0)
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
            case WsParseState::HttpHandshake:
                if (_trans->recvVec.size() >= 4) // \r\n\r\n is 4 bytes long
                {
                    auto ret = std::string::find(
                        &(_trans->recvVec)[0], _lastParsePos, 
                        _trans->recvVec.size());

                    if (ret == std::string::npos) // Not found.
                    {
                        _lastParsePos = recvVec.size() - 4;
                        return Codes::ST_NeedRecv;
                    }
                    else
                    {
                        // Found \r\n\r\n.
                        return parseHttpHandshake();
                    }
                }
                break;

            case WsParseState::HttpBody:
                break;

            case WsParseState::DataFrame:
                break;
            default:
                PARROT_ASSERT(false);
                break;
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
        http_parser_init(parser, HTTP_REQUEST);
        uint32_t ret = http_parser_execute(_parser, &settings,
                                           &(_trans->recvVec)[0],
                                           (_trans->recvBuff).size());

        // Client must send upgrade. Or we just disconnect.
        // Client can send upgrade with a body (RFC6455 allows). But if 
        // client sends upgrade with chunk data, it won't work.
        if (_parser->upgrade)
        {
            // Handle WebSocket.
            auto it = _headerDic.find("content-length");
            if (it == _headerDic.end())
            {
                // No body.
                _parseState = WsParseState::DataFrame;
            }
            else
            {
                _httpBodyLen = (it->second).stoul();
                if (_httpBodyLen > WsTranslayer::HTTP_HANDSHAKE_LEN)
                {
                    // The client should not send very big handshake packet.
                    return Codes::ERR_HttpHeader;
                }

                uint32_t receivedBodyLen = 0;
                auto ret = std::string::find(
                    &(_trans->recvVec)[0], _lastParsePos, 
                    _trans->recvVec.size());
                    
                ret += 4;
                receivedBodyLen = (_trans->recvBuff).size() - ret;

                if (bodyLen == receivedBodyLen)
                {
                    _parseState = WsParseState::DataFrame;
                }
                else
                {
                
                }
            }

        }
        else
        {
            return Codes::ERR_HttpHeader;
        }
    }

    Codes WsParser::checkHttpBody()
    {
        // Handle WebSocket.
        auto it = _headerDic.find("content-length");
        if (it == _headerDic.end())
        {
            // No body.
            _parseState = WsParseState::DataFrame;
        }
        else
        {
            uint32_t bodyLen = (it->second).stoul();
            if (bodyLen > WsTranslayer::HTTP_HANDSHAKE_LEN)
            {
                // The client should not send very big handshake packet.
                return Codes::ERR_HttpHeader;
            }

            uint32_t receivedBodyLen = 0;
            auto ret = std::string::find(
                &(_trans->recvVec)[0], _lastParsePos, 
                _trans->recvVec.size());
                    
            ret += 4;
            receivedBodyLen = (_trans->recvBuff).size() - ret;

            if (bodyLen == receivedBodyLen)
            {
                _parseState = WsParseState::DataFrame;
            }
            else
            {
                
            }
        }
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
        _headerDic[_lastHeaderField] = std::move(value);
        _lastHeaderField = "";
        return 0;
    }

    int WsParser::onHeaderComplete(http_parser *p)
    {
        if (!validSecWebSocketKey)
        {
            return 1;
        }
        
        
        delete _parser;
        _parser = nullptr;
    }

    bool WsParser::validSecWebSocketKey()
    {
        auto it = _headerDic.find("sec_websocket_key");
        if (it == _headerDic.end() || it->empty())
        {
            return false;
        }

        // According to RFC6455, we do not need to decode the key. 
        return true;
    }

    bool WsParser::
}
