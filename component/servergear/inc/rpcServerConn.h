#ifndef __COMPONENT_SERVERGEAR_INC_RPCSERVERCONN_H__
#define __COMPONENT_SERVERGEAR_INC_RPCSERVERCONN_H__

#include "wsServerConn.h"
#include "rpcSession.h"
#include "wsPacketHandler.h"

namespace parrot
{
struct WsConfig;
class RpcServerThread;

class RpcServerConn
    : public WsServerConn<RpcSession>,
      public WsPacketHandler<RpcSession, WsServerConn<RpcSession>>
{
  public:
    RpcServerConn(const WsConfig& cfg);

  public:
    void setRpcSrvThread(RpcServerThread* thread);

  public:
    void onPacket(std::shared_ptr<const RpcSession>&&,
                  std::unique_ptr<WsPacket>&&) override;
    void onClose(WsServerConn<RpcSession>* conn,
                 std::unique_ptr<WsPacket>&&) override;

  private:
    bool handshake(std::unique_ptr<WsPacket> &pkt);
    
  private:
    RpcServerThread* _rpcSrvThread;
    bool _registered;
};
}

#endif
