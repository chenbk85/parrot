#include <algorithm> // std::copy
#include <functional>
#include <system_error>
#include <iostream>

#include "ioEvent.h"
#include "httpParser.h"
#include "wsTranslayer.h"
#include "wsClientHandshake.h"
#include "logger.h"
#include "stringHelper.h"
#include "macroFuncs.h"
#include "wsConfig.h"
#include "mtRandom.h"
#include "digestHelper.h"
#include "stringHelper.h"
#include "base64.h"
#include "urlParser.h"

namespace parrot
{
WsClientHandshake::WsClientHandshake(WsTranslayer& tr)
    : _state(eParseState::CreateReq),
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
      _random(tr._random),
      _urlInfo(tr._io.getUrlInfo()),
      _config(tr._config)
{
}

int WsClientHandshake::onHeaderField(::http_parser*, const char* at, size_t len)
{
    if (!_lastHeader.empty())
    {
        return 1;
    }
    _lastHeader = std::string(at, len);
    strToLower(_lastHeader);
    return 0;
}

int WsClientHandshake::onHeaderValue(::http_parser*, const char* at, size_t len)
{
    _headerDic[_lastHeader] = std::string(at, len);
    _lastHeader             = "";
    return 0;
}

eCodes WsClientHandshake::parse()
{
    if (_rcvdLen < 4) // \r\n\r\n is 4 bytes long
    {
        return eCodes::ST_NeedRecv;
    }

    if (_rcvdLen >= _config._maxHttpHandshake)
    {
        LOG_WARN("WsClientHandshake::parse: Data too long. Remote is "
                 << _remoteIp << ".");
        _httpResult = eCodes::HTTP_PayloadTooLarge;
        return eCodes::ST_Ok;
    }

    auto end         = _recvVec.begin() + _rcvdLen;
    std::string rnrn = "\r\n\r\n";
    auto ret         = std::search(_lastParseIt, end, rnrn.begin(), rnrn.end());

    if (ret == end) // Not found.
    {
        _lastParseIt = end - 4; // Rewind 4 bytes for \r\n\r\n.

        if (_rcvdLen >= _recvVec.capacity())
        {
            // Http header length is longer than the buffer size.
            return eCodes::HTTP_PayloadTooLarge;
        }

        return eCodes::ST_NeedRecv;
    }

    // If here, we received http header.
    _lastParseIt = ret + 4; // Point to the end of the header.

    // Init settings.
    http_parser_settings settings;
    using namespace std::placeholders;
    settings.on_header_field =
        std::bind(&WsClientHandshake::onHeaderField, this, _1, _2, _3);
    settings.on_header_value =
        std::bind(&WsClientHandshake::onHeaderValue, this, _1, _2, _3);

    // Init parser.
    std::unique_ptr<::http_parser> parser(new ::http_parser());
    http_parser_init(parser.get(), HTTP_REQUEST);
    http_parser_execute(parser.get(), &settings,
                        reinterpret_cast<char*>(&_recvVec[0]), _rcvdLen);

    if ((parser->status_code !=
         static_cast<uint16_t>(eCodes::HTTP_SwitchingProtocols)) ||
        (parser->http_major != 1 && parser->http_major != 2) ||
        (parser->http_major == 1 && parser->http_minor != 1))
    {
        LOG_WARN("WsClientHandshake::parse: Failed to parse "
                 "header: "
                 << std::string(reinterpret_cast<char*>(&_recvVec[0]), _rcvdLen)
                 << ". Remote is " << _remoteIp);
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
        LOG_WARN("WsClientHandshake::parse: Body too long. "
                 "header: "
                 << std::string(reinterpret_cast<char*>(&_recvVec[0]), _rcvdLen)
                 << ". Remote is " << _remoteIp);
        _httpResult = eCodes::HTTP_PayloadTooLarge;
        return eCodes::ST_Ok;
    }

    _state = eParseState::RecevingBody;
    return eCodes::ST_NeedRecv;
}

eCodes WsClientHandshake::recevingBody()
{
    uint32_t rcvdLen = (_recvVec.begin() + _rcvdLen) - _lastParseIt;
    if (rcvdLen < _httpBodyLen)
    {
        return eCodes::ST_NeedRecv;
    }
    else if (rcvdLen > _httpBodyLen)
    {
        LOG_WARN("WsClientHandshake::recevingBody: Bad client. Remote is "
                 << _remoteIp);
        _httpResult = eCodes::HTTP_BadRequest;
        return eCodes::ST_Ok;
    }

    // Received http body.
    LOG_WARN("WsClientHandshake::recevingBody: Remote sent http body when "
             "handshaking. Remote is "
             << _remoteIp);

    return eCodes::ST_Ok;
}

bool WsClientHandshake::verifySecWebSocketAccept(const std::string& acceptedKey)
{
    using uchar = unsigned char;

    std::string swKey =
        _secWebSocketKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

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

    if (acceptedKey != std::string(&sha1Base64[0]))
    {
        return false;
    }

    return true;
}

void WsClientHandshake::verifyHeader()
{
    _httpResult = eCodes::HTTP_BadRequest;

    // Check upgrade.
    auto it = _headerDic.find("upgrade");
    if (it == _headerDic.end())
    {
        return;
    }

    if (it->second != "websocket")
    {
        return;
    }

    // Check connection.
    it = _headerDic.find("connection");
    if (it == _headerDic.end())
    {
        return;
    }

    if (it->second != "Upgrade")
    {
        return;
    }

    // Check sec-websocket-key
    it = _headerDic.find("sec-websocket-accept");
    if (it == _headerDic.end())
    {
        return;
    }

    if (!verifySecWebSocketAccept(it->second))
    {
        return;
    }

    _httpResult = eCodes::HTTP_Ok;
    return;
}

void WsClientHandshake::createHandshakeKey()
{
    using uchar      = unsigned char;
    uchar keyArr[16] = {'\0'};

    for (int i = 0; i != sizeof(keyArr); ++i)
    {
        keyArr[i] = _random->random(256);
    }

    uchar base64Key[32] = {'\0'};
    base64Encode(reinterpret_cast<char*>(&base64Key[0]),
                 reinterpret_cast<char*>(&keyArr[0]), sizeof(keyArr));

    _secWebSocketKey = reinterpret_cast<char*>(&base64Key[0]);
}

void WsClientHandshake::createHttpHandshake()
{
    std::ostringstream ostr;

    createHandshakeKey();
    
    ostr << "GET " << _urlInfo->_path << " HTTP/1.1\r\n"
         << "Host: " << _urlInfo->_authority << "\r\n"
         << "Upgrade: websocket\r\n"
         << "Connection: Upgrade\r\n"
         << "Sec-WebSocket-Version: 13\r\n"
         << "Sec-WebSocket-Key: " << _secWebSocketKey << "\r\n\r\n";

    std::string headerStr = ostr.str();
    std::copy_n(headerStr.begin(), headerStr.size(), &_sendVec[0]);
    _needSendLen = headerStr.size();
}

eCodes WsClientHandshake::getResult() const
{
    return _httpResult;
}

eCodes WsClientHandshake::work()
{
    eCodes code = eCodes::ST_Init;
    switch (_state)
    {
        case eParseState::CreateReq:
        {
            createHttpHandshake();
            _state = eParseState::Receving;
            return eCodes::ST_Ok;
        }
        break;

        case eParseState::Receving:
        {
            code = parse();
            if (code != eCodes::ST_Ok)
            {
                return code;
            }

            verifyHeader();
            return eCodes::ST_Complete;
        }
        break;

        case eParseState::RecevingBody:
        {
            code = recevingBody();

            if (code != eCodes::ST_Ok)
            {
                return code;
            }

            verifyHeader();
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
