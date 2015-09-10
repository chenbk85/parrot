#ifndef __COMPONENT_WEBSOCKET_INC_WSTRANSLAYER_H__
#define __COMPONENT_WEBSOCKET_INC_WSTRANSLAYER_H__

#include <list>
#include <memory>

#include <vector>
#include <cstdint>

#include "codes.h"

namespace parrot
{
    class WsTranslayer
    {
        friend class WsHttpResponse;

        using HeaderDic = std::unordered_map<std::string, std::string>;
        enum
        {
            HTTP_HANDSHAKE_LEN = 8192,
            SEND_BUFF_LEN = 65536,
            RECV_BUFF_LEN = 65536
        };

        enum TranslayerState
        {
            RecvHttpHandshake,
            RecvHttpBody,
            SendHttpHandshake,
            RecvDataFrame,
            SendDataFrame
        };

      public:
        WsTranslayer(IoEvent *io);
        ~WsTranslayer();
        WsTranslayer(const WsTranslayer &) = delete;
        WsTranslayer& operator=(const WsTranslayer &) = delete;

      public:
        void recvPacket();
        void getPacket();
        void sendPacket(std::unique_ptr<Packet> &&pkt);
        void sendPacket(std::list<std::unique_ptr<Packet>> &&pktList);
        
      private:
        TranslayerState                          _state;
        HeaderDic                                _headerDic;
        WebSocket &                              _io;
        std::list<std::unique_ptr<Packet>>       _pktList;

        WsParseState                             _parseState;
        std::vector<char>                        _sendVec;
        uint32_t                                 _sentLen;

        std::vector<char>                        _recvVec;

        uint32_t                                 _lastParsePos;
        uint32_t                                 _httpBodyLen;
        Codes                                    _httpResult;
        const WsConfig *                         _wsConfig;
    };
}

#endif
