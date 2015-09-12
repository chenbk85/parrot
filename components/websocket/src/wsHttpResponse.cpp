#include <algorithm>    // std::copy

#include "wsHttpResponse.h"
#include "logger.h"
#include "stringHelper.h"
#include "macroFuncs.h"

namespace parrot
{
    WsHttpResponse::WsHttpResponse(WsTranslayer &trans):
        _state(eParseState::Receving),
        _trans(trans),
        _headerDic(),
        _lastHeader(),
        _lastParsePos(0),
        _httpBodyLen(0),
        _httpResult(eCodes::HTTP_Ok)
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

    eCodes WsHttpResponse::parse()
    {
        if (_trans._recvVec.size() < 4) // \r\n\r\n is 4 bytes long
        {
            return eCodes::ST_NeedRecv;
        }

        if (_trans._recvVec.size() >= WsTranslayer::HTTP_HANDSHAKE_LEN)
        {
            LOG_WARN("WsHttpResponse::parse: Data too long. Remote is " <<
                     _trans.io.getRemoteAddr() << ".");
            _httpResult = eCodes::HTTP_PayloadTooLarge;
            return eCodes::ST_Ok;
        }

        auto start = &(_trans._recvVec)[0] + _lastParsePos;
        auto end = start + &_trans._recvVec.size();
        auto ret = std::find(start, end, "\r\n\r\n");

        if (ret == end) // Not found.
        {
            _lastParsePos = _trans._recvVec.size() - 4;
            return eCodes::ST_NeedRecv;
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
            _httpResult = eCodes::HTTP_BadRequest;
            return eCodes::ST_Ok;
        }

        auto it = _headerDic["content-length"];
        
        if (it == _headerDic.end())
        {
            return eCodes::ST_Ok;
        }

        _httpBodyLen = it->second.stoul();
        if (_httpBodyLen == 0)
        {
            return eCodes::ST_Ok;
        }

        if (_httpBodyLen >= WsTranslayer::HTTP_HANDSHAKE_LEN)
        {
            LOG_WARN("WsHttpResponse::parse: Body too long. "
                     "header: " << &(_trans._recvVec)[0] << ". Remote is " <<
                     _io->getRemoteAddr());
            _httpResult = eCodes::HTTP_PayloadTooLarge;
            return eCodes::ST_Ok;
        }

        _state = eParseState::RecevingBody;
        return eCodes::ST_NeedRecv;
    }

    eCodes WsHttpResponse::recevingBody()
    {
        uint32_t recvdLen = _trans._recvVec.size() - _lastParsePos;
        if (recvdLen < _httpBodyLen)
        {
            return eCodes::ST_NeedRecv;
        } 
        else if (recvdLen > _httpBodyLen)
        {
            LOG_WARN("WsHttpResponse::recevingBody: Bad client. Remote is " 
                     << _io->getRemoteAddr());
            _httpResult = eCodes::HTTP_BadRequest;
            return eCodes::ST_Ok;
        }

        // Received http body.
        LOG_WARN("WsHttpResponse::recevingBody: Remote sent http body when "
                 "handshaking. Remote is " << _io->getRemoteAddr());

        return eCodes::ST_Ok;
    }

    void WsHttpResponse::verifyHeader()
    {
        _httpResult = eCodes::HTTP_BadRequest;
        // Check host.
        auto it = _headerDic.find("host");
        if (it == _headerDic.end())
        {
            return;
        }

        if (it->second != _trans._wsConfig.host)
        {
            return;
        }

        // Check upgrade.
        it = _headerDic.find("upgrade");
        if (it == _headerDic.end())
        {
            return;
        }

        if (iStringFind("websocket") == std::string::npos)
        {
            return;
        }

        // Check connection.
        it = _headerDic.find("connection");
        if (it == _headerDic.end())
        {
            return;
        }

        if (iStringFind(it->second, "upgrade") == std::string::npos)
        {
            return;
        }
        
        // Check sec-websocket-key
        it = _headerDic.find("sec-websocket-key");
        if (it == _headerDic.end())
        {
            return;
        }

        // Client should first generate 16 bytes long buffer, then encode it to 
        // base64 string.
        if (getBase64DecodeLen(it->second.c_str()) - 1 != 16)
        {
            return;
        }

        // Check sec-websocket-version
        it = _headerDic.find("sec-websocket-version");
        if (it == _headerDic.end())
        {
            return;
        }

        // RFC6455 says this value must be 13.
        if (it->second.c_str() != "13")
        {
            _httpResult = eCodes::HTTP_UpgradeRequired;
            return;
        }

        _httpResult = eCodes::HTTP_SwitchingProtocols;
        return;
    }

    std::string WsTranslayer::createHandshakeSHA1Key()
    {
        using uchar = unsigned char;

        std::string swKey = _headerDic["sec-websocket-key"] + 
            "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        const uint32_t sha1BufLen = 20;
        uchar sha1Buf[sha1BufLen];

        eCodes code = sha1Message((uchar *)swKey.c_str(), swKey.size(), 
                                 &sha1Buf[0]);
        if (code != eCodes::ST_Ok)
        {
            PARROT_ASSERT(false);
        }

        // 2 times length is enough to hold base64 buffer.
        char sha1Base64[sha1BufLen << 1];
        base64Encode(sha1Base64, (const char*)&sha1Buf[0], sizeof(sha1Buf));
        return &sha1Base64[0];
    }

    void WsTranslayer::createHttpHandshakeRsp()
    {
        std::ostringstream ostr;
        std::system_error e(static_cast<int>(_httpResult), ParrotCategory());

        ostr << "HTTP/1.1 " << e.code().value() << " " 
             << e.code().messgae() << "\r\n";

        if (_httpResult != eCodes::HTTP_SwitchingProtocols)
        {
            ostr << "Connection: Closed\r\n\r\n";
        }
        else
        {
            ostr << "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                "Sec-WebSocket-Accept: " 
                 << createHandshakeSHA1Key() 
                 << "\r\n\r\n";
        }

        std::string headerStr = std::move(ostr.ostr());
        std::copy_n(headerStr.begin(), headerStr.size(), 
                    std::back_inserter(_trans._sendVec));
    }

    eCodes WsHttpResponse::work()
    {
        eCodes code = eCodes::ST_Init;
        switch (_state)
        {
            case eParseState::Receving:
            {
                code = parse();
                if (code != eCodes::ST_Ok)
                {
                    return code;
                }

                _state = eParseState::CreateRsp;
                return work();
            }
            break;

            case eParseState::RecevingBody:
            {
                code = recevingBody();

                if (code != eCodes::ST_Ok)
                {
                    return code;
                }

                _state = eParseState::CreateRsp;
                return work();
            }
            break;

            case eParseState::CreateRsp:
            {
                verifyHeader();
                createHttpHandshakeRsp();
                return eCodes::ST_Complete;
            }
            break;

            default:
            {
                PARROT_ASSERT(false);
            }
            break;
        }

        PARROT_ASSERT(false);
        return eCodes::ERR_Fail;
    }
}
