#ifndef __COMPONENT_WEBSOCKET_INC_WSHTTPRESPONSE_H__
#define __COMPONENT_WEBSOCKET_INC_WSHTTPRESPONSE_H__

namespace parrot
{
    class WsHttpResponse
    {
        using HeaderDic = std::unordered_map<std::string, std::string>;
        enum class eParseState
        {
            Receving,
                RecevingBody,
                CreateRsp
                };

      public:
        explicit WsHttpResponse(WsTranslayer &trans);

      public:

        // work
        //
        // A simple state machine. Receive http handshake message from
        // remote, parse it and create response message.
        //
        // return:
        //  * ST_NeedRecv       When we just received just part of 
        //                      handshake message.
        //  * ST_Complete       When created response.
        Codes work();

        // getResult
        //
        // Returns the http status code.
        //
        // return:
        //  * HTTP_PayloadTooLarge
        //  * HTTP_BadRequest
        //  * HTTP_UpgradeRequired
        //  * HTTP_SwitchingProtocols
        Codes getResult() const;

      private:
        // onUrl
        //
        // A callback function which will be called from http_parser when
        // the parser has parsed url.
        void onUrl(http_parser*, const char *at, size_t len);

        // onHeaderField
        //
        // A callback function which will be called from http_parser when
        // the parser has parsed one header.
        void onHeaderField(http_parser*, const char *at, size_t len);

        // onHeaderValue
        //
        // A callback function which will be called from http_parser when
        // the parser has parsed header value.
        void onHeaderValue(http_parser*, const char *at, size_t len);

        // parse
        //
        // Parse http handshake message.
        //
        // return:
        //  * ST_Ok             When parsing http message completed.
        //  * ST_NeedRecv       When only received just part of 
        //                      handshake message.
        Codes parse();

        // recevingBody
        //
        // RFC6455 says the client may send any valid http headers. Then, 
        // the received header may contains 'content-length'. If it has,
        // we have to receive the http body.
        //
        // return:
        //  * ST_Ok             When received completed http message.
        //  * ST_NeedRecv       When only received just part of 
        //                      handshake message.
        Codes recevingBody();

        // verifyHeader
        //
        // Verify http header.
        void verifyHeader();

        // createHandshakeSHA1Key
        //
        // Create Sec-WebSocket-Accept key. The server must derive it 
        // from the Sec-WebSocket-Key that the client sent. To get it, 
        // concatenate the client's Sec-WebSocket-Key and 
        // "258EAFA5-E914-47DA-95CA-C5AB0DC85B11" together (it's a 
        // "magic string"), take the SHA-1 hash of the result, and 
        // return the base64 encoding of the hash.
        //
        // return:
        //  The base64 encoded string.
        std::string createHandshakeSHA1Key();

        // createHttpHandshakeRsp
        //
        // Create the http response message.
        void createHttpHandshakeRsp();

      private:
        eParseState                              _state;
        WsTranslayer &                           _trans;
        HeaderDic                                _headerDic;
        std::string                              _lastHeader;
        uint32_t                                 _lastParsePos;
        uint32_t                                 _httpBodyLen;
        Codes                                    _httpResult;
    };
}



#endif
