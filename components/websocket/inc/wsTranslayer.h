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
        friend class WsParser;

        using HeaderDic = std::unordered_map<std::string, std::string>;
        enum
        {
            kHttpHandshakeLen = 8192,
            kSendBuffLen = 65536,
            kRecvBuffLen = 65536,
            kRecvMaxLen = 1 << 20 // 1 MB
        };

        enum eTranslayerState
        {
            RecvHttpHandshake,
            SendHttpHandshake,
            RecvDataFrame,
            SendDataFrame
        };

      public:
        WsTranslayer(IoEvent &io);
        ~WsTranslayer();
        WsTranslayer(const WsTranslayer &) = delete;
        WsTranslayer& operator=(const WsTranslayer &) = delete;

      public:
        void recvPacket();
        void getPacket();
        void sendPacket(std::unique_ptr<Packet> &&pkt);
        void sendPacket(std::list<std::unique_ptr<Packet>> &&pktList);
        
      private:
        eTranslayerState                         _state;
        WebSocket &                              _io;
        std::list<std::unique_ptr<Packet>>       _pktList;

        std::unique_ptr<WsHttpResponse>          _httpRsp;
        std::unique_ptr<WsParser>                _wsParser;
        WsParseState                             _parseState;
        std::vector<char>                        _sendVec;
        uint32_t                                 _sentLen;

        std::vector<char>                        _recvVec;

        const WsConfig &                         _wsConfig;
    };
}

#endif
