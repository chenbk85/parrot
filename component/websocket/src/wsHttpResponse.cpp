#include <algorithm> // std::copy
#include <functional>
#include <system_error>

#include "httpParser.h"
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
WsHttpResponse::WsHttpResponse(std::vector<char>& recvVec,
                               std::vector<char>& sendVec,
                               const std::string& remoteIp,
                               const WsConfig& cfg)
    : _state(eParseState::Receving),
      _recvVec(recvVec),
      _sendVec(sendVec),
      _remoteIp(remoteIp),
      _headerDic(),
      _lastHeader(),
      _lastParseIt(_recvVec.begin()),
      _httpBodyLen(0),
      _httpResult(eCodes::HTTP_Ok),
      _config(cfg)
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
    _lastHeader = "";
    return 0;
}

eCodes WsHttpResponse::parse()
{
    if (_recvVec.size() < 4) // \r\n\r\n is 4 bytes long
    {
        return eCodes::ST_NeedRecv;
    }

    if (_recvVec.size() >= _config._maxHttpHandshake)
    {
        LOG_WARN("WsHttpResponse::parse: Data too long. Remote is " << _remoteIp
                                                                    << ".");
        _httpResult = eCodes::HTTP_PayloadTooLarge;
        return eCodes::ST_Ok;
    }

    auto start = _lastParseIt;
    auto end = start + _recvVec.size();
    std::string rnrn = "\r\n\r\n";
    auto ret = std::search(_lastParseIt, end, rnrn.begin(), rnrn.end());

    if (ret == end) // Not found.
    {
        _lastParseIt = _recvVec.end() - 4;
        return eCodes::ST_NeedRecv;
    }

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
    http_parser_execute(parser.get(), &settings, &_recvVec[0], _recvVec.size());

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
                 << &_recvVec[0] << ". Remote is " << _remoteIp);
        _httpResult = eCodes::HTTP_BadRequest;
        return eCodes::ST_Ok;
    }

    auto it = _headerDic.find("content-length");
    if (it == _headerDic.end())
    {
        return eCodes::ST_Ok;
    }

    _httpBodyLen = std::stoul(it->second);
    if (_httpBodyLen == 0)
    {
        return eCodes::ST_Ok;
    }

    if (_httpBodyLen >= _config._maxHttpHandshake)
    {
        LOG_WARN("WsHttpResponse::parse: Body too long. "
                 "header: "
                 << &_recvVec[0] << ". Remote is " << _remoteIp);
        _httpResult = eCodes::HTTP_PayloadTooLarge;
        return eCodes::ST_Ok;
    }

    _state = eParseState::RecevingBody;
    return eCodes::ST_NeedRecv;
}

eCodes WsHttpResponse::recevingBody()
{
    uint32_t recvdLen = _recvVec.end() - _lastParseIt;
    if (recvdLen < _httpBodyLen)
    {
        return eCodes::ST_NeedRecv;
    }
    else if (recvdLen > _httpBodyLen)
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
        return;
    }

    if (it->second != _config._host)
    {
        return;
    }

    // Check upgrade.
    it = _headerDic.find("upgrade");
    if (it == _headerDic.end())
    {
        return;
    }

    if (iStringFind(it->second, "websocket") == std::string::npos)
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
    if (it->second != "13")
    {
        _httpResult = eCodes::HTTP_UpgradeRequired;
        return;
    }

    _httpResult = eCodes::HTTP_SwitchingProtocols;
    return;
}

std::string WsHttpResponse::createHandshakeSHA1Key()
{
    using uchar = unsigned char;

    std::string swKey = _headerDic["sec-websocket-key"] +
                        "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
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
    return &sha1Base64[0];
}

void WsHttpResponse::createHttpHandshakeRsp()
{
    std::ostringstream ostr;
    std::system_error e(static_cast<int>(_httpResult), ParrotCategory());

    ostr << "HTTP/1.1 " << e.code().value() << " " << e.code().message()
         << "\r\n";

    if (_httpResult != eCodes::HTTP_SwitchingProtocols)
    {
        ostr << "Connection: Closed\r\n\r\n";
    }
    else
    {
        ostr << "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                "Sec-WebSocket-Accept: "
             << createHandshakeSHA1Key() << "\r\n\r\n";
    }

    std::string headerStr = std::move(ostr.str());
    std::copy_n(headerStr.begin(), headerStr.size(),
                std::back_inserter(_sendVec));
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
