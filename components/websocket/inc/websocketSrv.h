#ifndef __COMPONENT_WEBSOCKET_INC_WEBSOCKETSRV_H__
#define __COMPONENT_WEBSOCKET_INC_WEBSOCKETSRV_H__

#include <memory>
#include <list>
#include <string>

#include "tcpServer.h"
#include "timeoutGuard.h"

namespace parrot
{
    class WebSocketSrv : public TcpServer, public TimeoutGuard
    {
      public:
        WebSocketSrv();
        ~WebSocketSrv() = default;
        WebSocketSrv(const WebSocketSrv&) = delete;
        WebSocketSrv& operator=(const WebSocketSrv &) = delete;

      public:
        void onOpen();
        void onPong();
        void onClose(Codes code);
        void onData(const std::vector<char>::iterator &begin,
                    const std::vector<char>::iterator &end);
        void sendPacket(std::unique_ptr<WsPacket> &&pkt);
        void sendPacket(std::list<std::unique_ptr<WsPacket>> &&pkt);
        void handleIoEvent() override;
        void closeWebSocket(const std::string &reason);

      private:
        std::unique_ptr<WsTranslayer<WebSocketSrv>>        _translayer;
    };
}

#endif
