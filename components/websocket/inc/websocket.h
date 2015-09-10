#ifndef __COMPONENT_WEBSOCKET_INC_WEBSOCKET_H__
#define __COMPONENT_WEBSOCKET_INC_WEBSOCKET_H__

#include <memory>
#include <list>

namespace parrot
{
    class WebSocket : public IoEvnet
    {
      public:
        WebSocket(PktHandler &hdr);
        ~WebSocket();
        WebSocket(const WebSocket&) = delete;
        WebSocket& operator=(const WebSocket &) = delete;

      public:

        void heartbeat();
        void onPacket(std::unique_ptr<WsPacket> &&pkt);
        void sendPacket(std::unique_ptr<WsPacket> &&pkt);
        void sendPacket(std::list<std::unique_ptr<WsPacket>> &&pkt);
        void handleIoEvent() override;
        void closeWebSocket();

      private:
        PktHandler &                      _pktHandler;
        WsTranslayer *                    _translayer;
    };
}

#endif
