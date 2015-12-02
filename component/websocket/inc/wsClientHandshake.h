#ifndef __COMPONENT_WEBSOCKET_INC_WSCLIENTHANDSHAKE_H__
#define __COMPONENT_WEBSOCKET_INC_WSCLIENTHANDSHAKE_H__

#include <unordered_map>
#include <string>
#include <vector>

#include "codes.h"

struct http_parser;

namespace parrot
{
struct UrlInfo;
struct WsConfig;
struct WsTranslayer;

class WsClientHandshake
{
    using HeaderDic = std::unordered_map<std::string, std::string>;
    enum class eParseState
    {
        CreateReq,        
        Receving,
        RecevingBody
    };

  public:
    explicit WsClientHandshake(WsTranslayer& tr);

  public:
    // A simple state machine. Receive http handshake message from
    // remote, parse it and create response message.
    //
    // return:
    //  * ST_NeedRecv       When we just received just part of
    //                      handshake message.
    //  * ST_Complete       When created response.
    eCodes work();

    // Returns the http status code.
    //
    // return:
    //  * HTTP_PayloadTooLarge
    //  * HTTP_BadRequest
    //  * HTTP_UpgradeRequired
    //  * HTTP_SwitchingProtocols
    eCodes getResult() const;

  private:
    // A callback function which will be called from http_parser when
    // the parser has parsed one header.
    //
    // return:
    //  0 continue parsing. 1 error.
    int onHeaderField(::http_parser*, const char* at, size_t len);

    // A callback function which will be called from http_parser when
    // the parser has parsed header value.
    //
    // return:
    //  0 continue parsing. 1 error.
    int onHeaderValue(::http_parser*, const char* at, size_t len);

    // Parse http handshake message.
    //
    // return:
    //  * ST_Ok             When parsing http message completed.
    //  * ST_NeedRecv       When only received just part of
    //                      handshake message.
    eCodes parse();

    // RFC6455 says the client may send any valid http headers. Then,
    // the received header may contains 'content-length'. If it has,
    // we have to receive the http body.
    //
    // return:
    //  * ST_Ok             When received completed http message.
    //  * ST_NeedRecv       When only received just part of
    //                      handshake message.
    eCodes recevingBody();

    // Verify http header.
    void verifyHeader();

    // Create Sec-WebSocket-Key. Generate 16 bytes array, then encode
    // the array to base64 string.
    void createHandshakeKey();

    // Create the http handshake message.
    void createHttpHandshake();

    // Verify Sec-WebSocket-Accept from server.
    bool verifySecWebSocketAccept(const std::string& acceptedKey);

  private:
    eParseState _state;
    std::vector<unsigned char>& _recvVec;
    const uint32_t& _rcvdLen;
    std::vector<unsigned char>& _sendVec;
    uint32_t& _needSendLen;
    const std::string& _remoteIp;
    HeaderDic _headerDic;
    std::string _lastHeader;
    std::vector<unsigned char>::iterator _lastParseIt;
    std::string _secWebSocketKey;
    uint32_t _httpBodyLen;
    eCodes _httpResult;
    MtRandom *_random;
    const UrlInfo * _urlInfo;
    const WsConfig& _config;
};
}

#endif
