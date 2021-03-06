#include "logger.h"
#include "json.h"
#include "rpcServerConn.h"
#include "rpcServerThread.h"
#include "threadJob.h"
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

bool RpcServerConn::handshake(std::unique_ptr<WsPacket>& pkt)
{
    if (!pkt->decode())
    {
        LOG_WARN("RpcServerConn::handshake: Failed to decode "
                 "handshake packet. Remote is "
                 << getRemoteAddr() << ".");
        return false;
    }

    auto json = pkt->getJson();
    if (!json->containsKey("/sid"))
    {
        LOG_WARN("RpcServerConn::handshake: Bad handshake packet. Remote is "
                 << getRemoteAddr() << ".");
        return false;
    }

    std::string sid;
    json->getValue("sid", sid);
    getSession()->setRemoteSid(sid);
    return true;
}

void RpcServerConn::onPacket(WsServerConn<RpcSession>*,
                             std::unique_ptr<WsPacket>&& pkt)
{
    if (!_registered)
    {
        if (handshake(pkt))
        {
            _rpcSrvThread->registerConn(getSession()->getRemoteSid(), this);
            _registered = true;
        }
        else
        {
            _rpcSrvThread->removeConn(this);
        }
    }
    else
    {
        auto sysJson = pkt->getSysJson();
        if (!sysJson || !sysJson->containsKey("/session"))
        {
            LOG_WARN("RpcServerConn::handshake: Bad sys json. Remote is "
                     << getRemoteAddr() << ". Session is "
                     << getSession()->toString() << ".");
            _rpcSrvThread->removeConn(this);
            return;
        }

        std::string cliSessionStr;
        sysJson->getValue("/session", cliSessionStr);
        std::unique_ptr<Json> cliSession(new Json());
        if (!cliSession->parse(cliSessionStr))
        {
            LOG_WARN("RpcServerConn::handshake: Failed to decode client "
                     "session. Remote is "
                     << getRemoteAddr() << ". Session is "
                     << getSession()->toString() << ".");
            _rpcSrvThread->removeConn(this);
            return;
        }

        auto mgr = Scheduler<RpcSession>::getInstance()->getJobManager(
            pkt->getRoute(), getSession());

        if (!mgr)
        {
            LOG_WARN("RpcServerConn::handshake: Failed to find job manager for "
                     "route "
                     << pkt->getRoute() << ". Remote is " << getRemoteAddr()
                     << ". Session is " << getSession()->toString() << ".");
            return;
        }

        _rpcSrvThread->addReqPacket(
            mgr, RpcSrvReqJobParam(std::move(getSession()),
                                   std::move(cliSession), std::move(pkt)));
    }
}

void RpcServerConn::onClose(WsServerConn<RpcSession>*,
                            std::unique_ptr<WsPacket>&&)
{
    LOG_WARN("RpcServerConn::onClose: Session is "
             << this->getSession()->toString() << ".");
    _rpcSrvThread->removeConn(this);
}
}
