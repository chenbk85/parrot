#include "wsHttpResponse.h"
#include "logger.h"
#include "stringHelper.h"
#include "macroFuncs.h"

namespace parrot
{
    WsHttpResponse::WsHttpResponse(WsTranslayer &trans):
        _state(eParseState::Receving),
        _trans(trans),
        _lastHeader(),
        _lastParsePos(0),
        _httpBodyLen(0),
        _httpResult(Codes::HTTP_Ok)
    {
    }

    void WsHttpResponse::onUrl(http_parser*, const char *at, size_t len)
    {
        _headerDic["url"] = std::string(at, len);
    }

    void WsHttpResponse::onHeaderField(http_parser*, const char *at, 
                                       size_t len)
    {
        PARROT_ASSERT(_lastHeader.empty());
        _lastHeader = std::string(at, len);
        strToLower(_lastHeader);
    }

    void WsHttpResponse::onHeaderValue(http_parser*, const char *at, 
                                       size_t len)
    {
        _headerDic[_lastHeader] = std::string(at, len);
        _lastHeader = "";
    }

    Codes WsHttpResponse::parse()
    {
        if (_trans._recvVec.size() < 4) // \r\n\r\n is 4 bytes long
        {
            return Codes::ST_NeedRecv;
        }

        if (_trans._recvVec.size() >= WsTranslayer::HTTP_HANDSHAKE_LEN)
        {
            LOG_WARN("WsHttpResponse::parse: Header too long. Remote is " <<
                     _trans.io.getRemoteAddr() << ".");
            return Codes::ERR_HttpHeader;
        }

        auto start = &(_trans._recvVec)[0] + _lastParsePos;
        auto end = start + &_trans._recvVec.size();
        auto ret = std::find(start, end, "\r\n\r\n");

        if (ret == end) // Not found.
        {
            _lastParsePos = _trans._recvVec.size() - 4;
            return Codes::ST_NeedRecv;
        }

        // If here, we received http header.

        _lastParsePos = ret + 4; // Point to the end of the header.

        // Init settings.
        http_parser_settings settings;
        settings.on_url = onUrl;
        settings.on_header_field = onHeaderField;
        settings.on_header_value = onHeaderValue;

        // Init parser.
        std::unique_ptr<http_parser> parser(new http_parser());
        http_parser_init(parser.get(), HTTP_REQUEST);
        http_parser_execute(parser.get(), &settings, &_recvVec[0],
                            _recvVec.size());

        // Client must send upgrade. Or we just disconnect.
        // Client can send upgrade with a body (RFC6455 allows). But if 
        // client sends upgrade with chunk data, it won't work.
        // Client must use HTTP/1.1 or above.
        if (!_parser->upgrade || 
            (_parser->http_major != 1 && _parser->http_major != 2) ||
            (_parser->http_major == 1 && _parser->http_minor != 1))
        {
            LOG_WARN("WsHttpResponse::parse: Failed to parse "
                     "header: " << &(_trans._recvVec)[0] << ". Remote is " <<
                     _io->getRemoteAddr());
            _httpResult = Codes::HTTP_BadRequest;
            return Codes::ST_Complete;
        }

        auto it = _headerDic["content-length"];
        
        if (it == _headerDic.end())
        {
            return Codes::ST_Ok;
        }

        _httpBodyLen = it->second.stoul();
        if (_httpBodyLen == 0)
        {
            return Codes::ST_Ok;
        }

        if (_httpBodyLen >= WsTranslayer::HTTP_HANDSHAKE_LEN)
        {
            LOG_WARN("WsHttpResponse::parse: Body too long. "
                     "header: " << &(_trans._recvVec)[0] << ". Remote is " <<
                     _io->getRemoteAddr());
            return Codes::ERR_HttpHeader;
        }

        _state = eParseState::RecevingBody;
        return Codes::ST_NeedRecv;
    }

    Codes WsHttpResponse::recevingBody()
    {
        uint32_t recvdLen = _trans._recvVec.size() - _lastParsePos;
        if (recvdLen < _httpBodyLen)
        {
            return Codes::ST_NeedRecv;
        } 
        else if (recvdLen > _httpBodyLen)
        {
            LOG_WARN("WsHttpResponse::recevingBody: Bad client. Remote is " 
                     << _io->getRemoteAddr());
            return Codes::ERR_Fail;
        }

        // Received http body.
        LOG_WARN("WsHttpResponse::recevingBody: Remote sent http body when "
                 "handshaking. Remote is " << _io->getRemoteAddr());

        return Codes::ST_Ok;
    }

    Codes WsHttpResponse::verifyHeader()
    {
        _httpResult = Codes::HTTP_BadRequest;
        // Check host.
        auto it = _headerDic.find("host");
        if (it == _headerDic.end())
        {
            return false;
        }

        if (it->second != _wsConfig->host)
        {
            return false;
        }

        // Check upgrade.
        it = _headerDic.find("upgrade");
        if (it == _headerDic.end())
        {
            return false;
        }

        if (iStringFind("websocket") == std::string::npos)
        {
            return false;
        }

        // Check connection.
        it = _headerDic.find("connection");
        if (it == _headerDic.end())
        {
            return false;
        }

        if (iStringFind(it->second, "upgrade") == std::string::npos)
        {
            return false;
        }
        
        // Check sec-websocket-key
        it = _headerDic.find("sec-websocket-key");
        if (it == _headerDic.end())
        {
            return false;
        }

        // Client should first generate 16 bytes long buffer, then encode it to 
        // base64 string.
        if (getBase64DecodeLen(it->second.c_str()) - 1 != 16)
        {
            return false;
        }

        // Check sec-websocket-version
        it = _headerDic.find("sec-websocket-version");

        if (it == _headerDic.end())
        {
            return false;
        }

        // RFC6455 says this value must be 13.
        if (it->second.c_str() != "13")
        {
            _httpResult = Codes::HTTP_UpgradeRequired;
            return false;
        }

        return true;
    }

    std::string WsTranslayer::createHandshakeSHA1Key()
    {
        using uchar = unsigned char;

        std::string catKey = _headerDic["sec-websocket-key"] + 
            "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        uchar sha1Buf[20];

        Codes code = sha1Message((uchar *)catKey.c_str(), catKey.size(), 
                                 &sha1Buf[0]);
        if (code != Codes::ST_Ok)
        {
            return "";
        }

        char sha1Res[41];
        binToHexLowCase(&sha1Buf[0], sizeof(sha1Buf), &sha1Res[0]);
        return &sha1Res[0];
    }

    void WsTranslayer::createHttpHandshakeRsp()
    {
        std::ostringstream ostr;
        ostr << "HTTP/1.1 101 Switching Protocols\r\n"
             << "Upgrade: websocket\r\n"
             << "Connection: Upgrade\r\n"
             << "Sec-WebSocket-Accept: " << createHandshakeSHA1Key() << "\r\n";

        // Check connection.
        auto it = _headerDic.find("sec-websocket-protocol");
        if (it != _headerDic.end())
        {
            // We speak parrot language.
            if (iStringFind(it->second, "parrot") != std::string::npos)
            {
                ostr << "Sec-WebSocket-Protocol: parrot\r\n";
            }
        }


        std::string headerStr = std::move(ostr.ostr());
        std::strcpy(&_sendVec[0], headerStr.c_str(), headerStr.size());
        _sendVec.resize(headerStr.size());
    }

    Codes WsHttpResponse::work()
    {
        Codes code = Codes::ST_Init;
        switch (_state)
        {
            case eParseState::Receving:
            {
                code = parse();
                if (code != Codes::ST_Ok)
                {
                    return code;
                }

                _state = eParseState::VerifyHeader;
                return work();
            }
            break;

            case eParseState::RecevingBody:
            {
                code = recevingBody();

                if (code != Codes::ST_Ok)
                {
                    return code;
                }

                _state = eParseState::VerifyHeader;
                return work();
            }
            break;

            case eParseState::VerifyHeader:
            {
                return verifyHeader();
            }
            break;

            case eParseState::createPsp:
            {

            }
            break;

            default:
            {
                PARROT_ASSERT(false);
            }
            break;
        }

        PARROT_ASSERT(false);
        return Codes::ERR_Fail;
    }
}
