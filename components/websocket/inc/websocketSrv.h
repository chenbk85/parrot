#ifndef __COMPONENT_WEBSOCKET_INC_WEBSOCKETSRV_H__
#define __COMPONENT_WEBSOCKET_INC_WEBSOCKETSRV_H__

#include <memory>
#include <list>
#include <vector>
#include <string>

#include "wsParser.h"

#include "codes.h"
#include "tcpServer.h"
#include "timeoutGuard.h"

namespace parrot {
class WsPacket;
class WsConfig;

template <typename T> class WsTranslayer;

class WebSocketSrv : public TcpServer, public TimeoutGuard {
  public:
    explicit WebSocketSrv(const WsConfig &cfg);
    virtual ~WebSocketSrv() = default;
    WebSocketSrv(const WebSocketSrv &) = delete;
    WebSocketSrv &operator=(const WebSocketSrv &) = delete;

  public:
    void onOpen();
    void onPong();
    void onClose(eCodes code);
    void onData(WsParser::eOpCode code, std::vector<char>::iterator begin,
                std::vector<char>::iterator end);
    void sendPacket(std::unique_ptr<WsPacket> &pkt);
    void sendPacket(std::list<std::unique_ptr<WsPacket>> &pkt);
    eIoAction handleIoEvent() override;
    void closeWebSocket(const std::string &reason);

  private:
    std::unique_ptr<WsTranslayer<WebSocketSrv>> _translayer;
};
}

#endif
