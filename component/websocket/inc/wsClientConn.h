#ifndef __COMPONENT_WEBSOCKET_INC_WSCLIENTCONN_H__
#define __COMPONENT_WEBSOCKET_INC_WSCLIENTCONN_H__

#include <memory>
#include <list>
#include <vector>
#include <ctime>
#include <string>

#include "codes.h"
#include "session.h"
#include "wsDefinition.h"
#include "tcpClientConn.h"
#include "timeoutGuard.h"
#include "wsPacketHandler.h"
#include "wsPacket.h"
#include "doubleLinkedListNode.h"
#include "wsClientTrans.h"

namespace parrot
{
class WsClientConn : public TcpClientConn,
                     public TimeoutGuard,
                     public DoubleLinkedListNode<WsClientConn>
{
    using PacketHandler = WsPacketHandler<Session, WsClientConn>;


    enum class eWsConnState
    {
        Disconnected,
        WaitingToConnect,
        Connecting,
        Connected
    };
    
    enum class eWsState
    {
        NotOpened,
        Opened,
        Closing
    };

  public:
    WsClientConn() = default;
    WsClientConn(const std::string& ip,
                 uint16_t port,
                 const WsConfig& cfg,
                 bool sendMasked = true);
    virtual ~WsClientConn() = default;
    WsClientConn(const WsClientConn&) = delete;
    WsClientConn& operator=(const WsClientConn&) = delete;

  public:
    void setPacketHandler(PacketHandler* hdr);
    std::shared_ptr<Session>& getSession();
    void sendPacket(std::unique_ptr<WsPacket>& pkt);
    void sendPacket(std::list<std::unique_ptr<WsPacket>>& pkt);
    eIoAction handleIoEvent() override;
    // Up layer close the socket, we should sent the close packet and
    // disconnect the connection.
    void closeWebSocket(std::unique_ptr<WsPacket>& pkt);
    void setRandom(MtRandom* random);
    bool canSwitchToSend() const;

  private:
    void onError(eCodes c);
    void onPacket(std::unique_ptr<WsPacket>&& pkt);
    void onConnected();
    void getNextConnectTime();    
    void onOpen();
    void onPing();
    void onPong();
    void onClose(std::unique_ptr<WsPacket>&& pkt);
    void onData(std::unique_ptr<WsPacket>&& pkt);
    bool canClose();
    void doClose();
    void onDisconnect();

  private:
    eWsConnState _connState;
    eWsState _state;
    PacketHandler* _pktHandler;
    std::shared_ptr<Session> _session;
    std::unique_ptr<WsClientTrans> _translayer;
    bool _sentClose;
    uint32_t _retryTimes;
    std::time_t _nextConnectTime;
    
};
}

#endif