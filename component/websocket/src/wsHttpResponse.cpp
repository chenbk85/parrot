#include <algorithm> // std::copy
#include <functional>
#include <system_error>
#include <iostream>

#include "ioEvent.h"
#include "httpParser.h"
#include "wsTranslayer.h"
#include "wsHttpResponse.h"
#include "logger.h"
#include "stringHelper.h"
#include "macroFuncs.h"
#include "wsConfig.h"
#include "digestHelper.h"
#include "stringHelper.h"
#include "base64.h"

namespace parrot
{
WsHttpResponse::WsHttpResponse(WsTranslayer& tr)
    : _state(eParseState::Receving),
      _recvVec(tr._recvVec),
      _rcvdLen(tr._rcvdLen),
      _sendVec(tr._sendVec),
      _needSendLen(tr._needSendLen),
      _remoteIp(tr._io.getRemoteAddr()),
      _headerDic(),
      _lastHeader(),
      _lastParseIt(_recvVec.begin()),
      _httpBodyLen(0),
      _httpResult(eCodes::HTTP_Ok),
      _config(tr._config)
{
}

int WsHttpResponse::onUrl(::http_parser*, const char* at, size_t len)
{
    _headerDic["url"] = std::string(at, len);
    return 0;
}

int WsHttpResponse::onHeaderField(::http_parser*, const char* at, size_t len)
{
    if (!_lastHeader.empty())
    {
        return 1;
    }
    _lastHeader = std::string(at, len);
    strToLower(_lastHeader);
    return 0;
}

int WsHttpResponse::onHeaderValue(::http_parser*, const char* at, size_t len)
{
    _headerDic[_lastHeader] = std::string(at, len);
    _lastHeader             = "";
    return 0;
}

eCodes WsHttpResponse::parse()
{
    std::cout << 1 << std::endl;
    if (_rcvdLen < 4) // \r\n\r\n is 4 bytes long
    {
        return eCodes::ST_NeedRecv;
    }

    std::cout << 2 << std::endl;
    if (_rcvdLen >= _config._maxHttpHandshake)
    {
        LOG_WARN("WsHttpResponse::parse: Data too long. Remote is " << _remoteIp
                                                                    << ".");
        _httpResult = eCodes::HTTP_PayloadTooLarge;
        return eCodes::ST_Ok;
    }

    auto end         = _recvVec.begin() + _rcvdLen;
    std::string rnrn = "\r\n\r\n";
    auto ret         = std::search(_lastParseIt, end, rnrn.begin(), rnrn.end());

    std::cout << 3 << std::endl;
    if (ret == end) // Not found.
    {
        _lastParseIt = end - 4; // Rewind 4 bytes for \r\n\r\n.
        return eCodes::ST_NeedRecv;
    }

    std::cout << 4 << std::endl;
    // If here, we received http header.
    _lastParseIt = ret + 4; // Point to the end of the header.

    // Init settings.
    http_parser_settings settings;
    using namespace std::placeholders;
    settings.on_url = std::bind(&WsHttpResponse::onUrl, this, _1, _2, _3);
    settings.on_header_field =
        std::bind(&WsHttpResponse::onHeaderField, this, _1, _2, _3);
    settings.on_header_value =
        std::bind(&WsHttpResponse::onHeaderValue, this, _1, _2, _3);

    // Init parser.
    std::unique_ptr<::http_parser> parser(new ::http_parser());
    http_parser_init(parser.get(), HTTP_REQUEST);
    http_parser_execute(parser.get(), &settings, &_recvVec[0], _rcvdLen);

    std::cout << 5 << std::endl;

    // Client must send upgrade. Or we just disconnect.
    // Client can send upgrade with a body (RFC6455 allows). But if
    // client sends upgrade with chunk data, it won't work.
    // Client must use HTTP/1.1 or above.
    if (!parser->upgrade ||
        (parser->http_major != 1 && parser->http_major != 2) ||
        (parser->http_major == 1 && parser->http_minor != 1))
    {
        LOG_WARN("WsHttpResponse::parse: Failed to parse "
                 "header: "
                 << std::string(&_recvVec[0], _rcvdLen) << ". Remote is "
                 << _remoteIp);
        _httpResult = eCodes::HTTP_BadRequest;
        return eCodes::ST_Ok;
    }

    std::cout << 6 << std::endl;

    auto it = _headerDic.find("content-length");
    if (it == _headerDic.end())
    {
        return eCodes::ST_Ok;
    }

    std::cout << 7 << std::endl;

    _httpBodyLen = std::stoul(it->second);
    if (_httpBodyLen == 0)
    {
        return eCodes::ST_Ok;
    }

    std::cout << 8 << std::endl;

    if (_httpBodyLen >= _config._maxHttpHandshake)
    {
        LOG_WARN("WsHttpResponse::parse: Body too long. "
                 "header: "
                 << std::string(&_recvVec[0], _rcvdLen) << ". Remote is "
                 << _remoteIp);
        _httpResult = eCodes::HTTP_PayloadTooLarge;
        return eCodes::ST_Ok;
    }

    std::cout << 9 << std::endl;

    _state = eParseState::RecevingBody;
    return eCodes::ST_NeedRecv;
}

eCodes WsHttpResponse::recevingBody()
{
    uint32_t rcvdLen = (_recvVec.begin() + _rcvdLen) - _lastParseIt;
    if (rcvdLen < _httpBodyLen)
    {
        return eCodes::ST_NeedRecv;
    }
    else if (rcvdLen > _httpBodyLen)
    {
        LOG_WARN("WsHttpResponse::recevingBody: Bad client. Remote is "
                 << _remoteIp);
        _httpResult = eCodes::HTTP_BadRequest;
        return eCodes::ST_Ok;
    }

    // Received http body.
    LOG_WARN("WsHttpResponse::recevingBody: Remote sent http body when "
             "handshaking. Remote is "
             << _remoteIp);

    return eCodes::ST_Ok;
}

void WsHttpResponse::verifyHeader()
{
    _httpResult = eCodes::HTTP_BadRequest;
    // Check host.
    auto it = _headerDic.find("host");
    if (it == _headerDic.end())
    {
        std::cout << "v1" << std::endl;
        return;
    }

    if (it->second != _config._host)
    {
        std::cout << "v2" << std::endl;
        return;
    }

    // Check upgrade.
    it = _headerDic.find("upgrade");
    if (it == _headerDic.end())
    {
        std::cout << "v3" << std::endl;
        return;
    }

    if (iStringFind(it->second, "websocket") == std::string::npos)
    {
        std::cout << "v4" << std::endl;
        return;
    }

    // Check connection.
    it = _headerDic.find("connection");
    if (it == _headerDic.end())
    {
        std::cout << "v5" << std::endl;
        return;
    }

    if (iStringFind(it->second, "upgrade") == std::string::npos)
    {
        std::cout << "v6" << std::endl;
        return;
    }

    // Check sec-websocket-key
    it = _headerDic.find("sec-websocket-key");
    if (it == _headerDic.end())
    {
        std::cout << "v7" << std::endl;
        return;
    }

    // Client should first generate 16 bytes long buffer, then encode it to
    // base64 string.
    if (it->second.length() >= 25)
    {
        return;
    }
    char tmpOut[32];
    if (base64Decode(&tmpOut[0], it->second.c_str()) != 16)
    {
        std::cout << "v8" << std::endl;
        return;
    }

    // Check sec-websocket-version
    it = _headerDic.find("sec-websocket-version");
    if (it == _headerDic.end())
    {
        std::cout << "v9" << std::endl;
        return;
    }

    // RFC6455 says this value must be 13.
    if (it->second != "13")
    {
        std::cout << "v10" << std::endl;
        _httpResult = eCodes::HTTP_UpgradeRequired;
        return;
    }

    std::cout << "v11" << std::endl;
    _httpResult = eCodes::HTTP_SwitchingProtocols;
    return;
}

std::string WsHttpResponse::createHandshakeSHA1Key()
{
    using uchar = unsigned char;

    std::string swKey = _headerDic["sec-websocket-key"] +
                        "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    std::cout << swKey << std::endl;
    
    const uint32_t sha1BufLen = 20;
    uchar sha1Buf[sha1BufLen];

    eCodes code = sha1Message((uchar*)swKey.c_str(), swKey.size(), &sha1Buf[0]);
    if (code != eCodes::ST_Ok)
    {
        PARROT_ASSERT(false);
    }

    // 2 times length is enough to hold base64 buffer.
    char sha1Base64[sha1BufLen << 1];
    base64Encode(sha1Base64, (const char*)&sha1Buf[0], sizeof(sha1Buf));

    std::cout << "key is: " << &sha1Base64[0] << std::endl;
    return &sha1Base64[0];
}

void WsHttpResponse::createHttpHandshakeRsp()
{
    std::ostringstream ostr;
    std::system_error e(static_cast<int>(_httpResult), ParrotCategory());

    if (_httpResult != eCodes::HTTP_SwitchingProtocols)
    {
        ostr << "HTTP/1.1 " << e.code().value() << " " << e.code().message()
             <<"\r\n";
        ostr << "Connection: Closed\r\n\r\n";
    }
    else
    {
        ostr << "HTTP/1.1 101 Switching Protocols\r\n"
                "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                "Sec-WebSocket-Accept: "
             << createHandshakeSHA1Key() << "\r\n\r\n";
    }

    std::cout << "Response: " << ostr.str() << std::endl;

    std::string headerStr = std::move(ostr.str());
    std::copy_n(headerStr.begin(), headerStr.size(), &_sendVec[0]);
    _needSendLen = headerStr.size();
}

eCodes WsHttpResponse::getResult() const
{
    return _httpResult;
}

eCodes WsHttpResponse::work()
{
    eCodes code = eCodes::ST_Init;
    switch (_state)
    {
        case eParseState::Receving:
        {
            code = parse();
            std::cout << "WsHttpResponse::work: " << code << std::endl;
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
