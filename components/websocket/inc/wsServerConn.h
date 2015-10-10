#ifndef __COMPONENTS_WEBSOCKET_INC_WSSERVERCONN_H__
#define __COMPONENTS_WEBSOCKET_INC_WSSERVERCONN_H__

#include <memory>
#include <list>
#include <vector>
#include <string>

#include "codes.h"
#include "wsDefinition.h"
#include "tcpServer.h"
#include "timeoutGuard.h"

namespace parrot
{
class WsPacket;
class WsTranslayer;
struct WsConfig;

class WsServerConn : public TcpServer, public TimeoutGuard
{
    enum class WsState
    {
        NotOpened,
        Opened,
        Closing
    };

  public:
    explicit WsServerConn(const WsConfig& cfg);
    virtual ~WsServerConn() = default;
    WsServerConn(const WsServerConn&) = delete;
    WsServerConn& operator=(const WsServerConn&) = delete;

  public:
    void sendPacket(std::unique_ptr<WsPacket>& pkt);
    void sendPacket(std::list<std::unique_ptr<WsPacket>>& pkt);
    eIoAction handleIoEvent() override;
    // Up layer close the socket, we should sent the close packet and
    // disconnect the connection.
    void closeWebSocket(std::unique_ptr<WsPacket>& pkt);

  private:
    void onError(eCodes c);
    void onPacket(std::unique_ptr<WsPacket> &&pkt);
    
    void onOpen();
    void onPing();
    void onPong();
    void onClose(std::unique_ptr<WsPacket> &&pkt);
    void onData(std::unique_ptr<WsPacket> &&pkt);
    bool canClose();
    void doClose();

  private:
    WsState _state;
    std::unique_ptr<WsTranslayer> _translayer;
    bool _sentClose;
};
}

#endif
