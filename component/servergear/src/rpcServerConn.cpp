#include "logger.h"
#include "rpcServerConn.h"
#include "rpcServerThread.h"
#include "wsConfig.h"

namespace parrot
{
RpcServerConn::RpcServerConn(const WsConfig& cfg)
    : WsServerConn<RpcSession>(cfg, false),
      WsPacketHandler<RpcSession, WsServerConn<RpcSession>>(),
      _rpcSrvThread(nullptr),
      _registered(false)
{
    setPacketHandler(this);
}

void RpcServerConn::setRpcSrvThread(RpcServerThread* thread)
{
    _rpcSrvThread = thread;
}

bool RpcServerConn::verifyPeer(std::unique_ptr<WsPacket> &)
{
    return true;
}

void RpcServerConn::onPacket(std::shared_ptr<const RpcSession>&&,
                             std::unique_ptr<WsPacket>&& pkt)
{
    if (!_registered)
    {
        if (verifyPeer(pkt))
        {
            // TODO: set remote sid.
            getSession()->setRemoteSid("sid");

            _rpcSrvThread->registerRpcClient(this);
            _registered = true;
        }
        else
        {
            // TODO: disconnect.
        }
    }
    else
    {
    }
}

void RpcServerConn::onClose(WsServerConn<RpcSession>* conn,
                            std::unique_ptr<WsPacket>&&)
{
}
}
