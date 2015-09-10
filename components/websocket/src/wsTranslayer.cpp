#include <algorithm>
#include <iterator>

#include <openssl/sha1.h>

#include "ioEvent.h"
#include "wsPacket.h"
#include "wsParser.h"
#include "wsTranslayer.h"
#include "logger.h"
#include "base64.h"
#include "stringHelper.h"
#include "digestHelper.h"

namespace parrot
{
    WsTranslayer::WsTranslayer():
        _sendVec(SEND_BUFF_LEN),
        _sentLen(0),
        _recvVec(RECV_BUFF_LEN),
        _rcvdLen(0),
        _needRecvLen(0),
        _io(nullptr)
    {
        _sendVec.resize(0);
        _recvVec.resize(0);
    }

    WsTranslayer::~WsTranslayer()
    {
    }

    Codes WsTranslayer::recvData()
    {
        uint32_t rcvdLen = 0;
        Codes code = _io->recv(&_recvVec[_rcvdLen], 
                               _recvVec.capacity() - _recvVec.size(), 
                               rcvdLen);

        if (code == Codes::ST_Ok)
        {
            _rcvdLen += rcvdLen;
            code = _parser->parse();
        }
        
        return code;
    }

    Codes WsTranslayer::sendData()
    {
        uint32_t sentLen = 0;
        Codes code = _io->send(&_sendVec[_sentLen], 
                               _sendVec.size() - _sentLen, 
                               sentLen);

        if (code == Codes::ST_Ok)
        {
            _sentLen += sentLen;
            if (_sentLen == _sendVec.size())
            {
                _sendVec.resize(0);
            }
        }
        
        return code;        
    }

    Codes WsTranslayer::sendPacket(
        std::list<std::shared_ptr<WsPacket>> &pktList)
    {
        for (auto it = pktList.begin(); it != pktList.end(); ++it)
        {
            std::copy_n((*it)->getBuffer(), (*it)->length(), 
                        std::back_inserter(_sendVec));
        }

        if (_sendVec.size() > SEND_BUFF_LEN)
        {
            LOG_WARN("WsTranslayer::sendPacket: _sendVec reallocated. "
                     "Try to change the SEND_BUFF_LEN to a bigger value. "
                     "Current size is " << _sendVec.size() << ".");
        }
    }

    Codes WsTranslayer::sendPacket(const WsPacket &pkt)
    {
        std::copy_n(pkt.getBuffer(), pkt.length(), 
                    std::back_inserter(_sendVec));

        if (_sendVec.size() > SEND_BUFF_LEN)
        {
            LOG_WARN("WsTranslayer::sendPacket: _sendVec reallocated. "
                     "Try to change the SEND_BUFF_LEN to a bigger value. "
                     "Current size is " << _sendVec.size() << ".");
        }
    }

    bool WsTranslayer::checkHttpHeader()
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

    Codes WsTranslayer::parseHttpHandshake()
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
        std::unique_ptr<http_parser> parser(new http_parser());
        http_parser_init(parser, HTTP_REQUEST);
        uint32_t ret = http_parser_execute(parser, &settings, &_recvVec[0],
                                           _recvVec.size());

        // Client must send upgrade. Or we just disconnect.
        // Client can send upgrade with a body (RFC6455 allows). But if 
        // client sends upgrade with chunk data, it won't work.
        // Client must use HTTP/1.1 or above.
        if (!_parser->upgrade || 
            (_parser->http_major != 1 && _parser->http_major != 2) ||
            (_parser->http_major == 1 && _parser->http_minor != 1))
        {
            LOG_WARN("WsTranslayer::parseHttpHandshake: Failed to parse "
                     "header: " << &_recvVec[0] << ". Remote is " <<
                     _io->getRemoteAddr());
            _httpResult = Codes::HTTP_BadRequest;
            return Codes::ERR_HttpHeader;
        }

        if (!checkHttpHeader())
        {
            LOG_WARN("WsTranslayer::parseHttpHandshake: Check header "
                     "failed: " << &_recvVec[0] << ". Remote is " <<
                     _io->getRemoteAddr());
            return Codes::ERR_HttpHeader;
        }

        // Check if body exists.
        auto it = _headerDic.find("content-length");
        if (it == _headerDic.end())
        {
            createHttpHandshake();
            _state = SendHttpHandshake;
            return Codes::ST_Received;
        }
        else
        {
            _httpBodyLen = (it->second).stoul();
            if (_httpBodyLen > WsTranslayer::HTTP_HANDSHAKE_LEN)
            {
                // The client should not send very big handshake packet.
                LOG_WARN("WsTranslayer::parseHttpHandshake: Http handshake "
                     "too big: " << &_recvVec[0] << ". Remote is " <<
                     _io->getRemoteAddr());
                return Codes::ERR_HttpHeader;
            }

            uint32_t receivedBodyLen = 0;
            receivedBodyLen = (_trans->recvBuff).size() - _lastParsePos;

            if (bodyLen == receivedBodyLen)
            {
                return Codes::ST_Received;
            }
            else
            {
                return Codes::ST_NeedRecv;
            }
        }
    }

    Codes WsTranslayer::parseHttpBody()
    {
        if (_recvVec.size() - _lastParsePos < _httpBodyLen)
        {
            return Codes::ST_NeedRecv;
        }

        return Codes::ST_Received;
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

    eIoAction WsTranslayer::work()
    {
        eIoAction act = eIoAction::None;
        Codes code = Codes::ST_Init;
        switch (_state)
        {
            case RecvHttpHandshake:
                recvData();
                if (_recvVec.size() < 4) // \r\n\r\n is 4 bytes long
                {
                    return eIoAction::Read;
                }

                auto start = &_recvVec[0] + _lastParsePos;
                auto end = start + _recvVec.size();
                auto ret = std::find(start, end, "\r\n\r\n");

                if (ret == end) // Not found.
                {
                    _lastParsePos = recvVec.size() - 4;
                    return eIoAction::Read;
                }

                // If here, received http header.

                _lastParsePos = ret + 4; // Point to the end of the http header.
                code = parseHttpHandshake();

                if (code == Codes::ST_Received)
                {
                    _state = SendHttpHandshake;
                    return work();
                }
                else if (code == Codes::ST_NeedRecv)
                {
                    _state = RecvHttpBody;
                    return eIoAction::Read;
                }
                else
                {
                    return eIoAction::Remove;
                }
                break;

            case RecvHttpBody:
                recvData();
                code = parseHttpBody();
                if (code == Codes::ST_Received)
                {
                    _state = SendHttpHandshake;
                    return work();
                }
                else
                {
                    return eIoAction::Read;
                }
                break;

            case SendHttpHandshake:
                act = sendData();
                break;

            case RecvDataFrame:
                recvData();
                // No break;

            case SendDataFrame:
                act = sendData();
                break;

            default:
                PARROT_ASSERT(false);
                break;
        }

        return act;
    }
}
