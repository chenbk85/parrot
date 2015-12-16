#include "logger.h"
#include "json.h"
#include "rpcClientConn.h"
#include "config.h"
#include "wsConfig.h"

namespace parrot
{
RpcClientConn::RpcClientConn(const Config& cfg,
                             const std::string& wsUrl,
                             const WsConfig& wscfg)
    : WsClientConn<RpcSession>(wsUrl, wscfg, false),
      _timeoutMgr(new TimeoutManager<RpcRequest>(this, cfg._rpcReqTimeout)),
      _reqMap(),
      _reqId(0),
      _config(cfg)
{
}

void RpcClientConn::addJob(std::unique_ptr<RpcRequest>&& req)
{
    if (req->getPacketType() == ePacketType::Request)
    {
        req->setReqId(_reqId);
        _timeoutMgr->add(req.get(), std::time(nullptr));
        _reqMap.emplace(_reqId, std::move(req));
        ++_reqId;
    }

    sendPacket(req->getPacket());
}

void RpcClientConn::addJob(std::list<std::unique_ptr<RpcRequest>>& reqList)
{
    auto now = std::time(nullptr);
    for (auto& req : reqList)
    {
        if (req->getPacketType() == ePacketType::Request)
        {
            req->setReqId(_reqId);
            _timeoutMgr->add(req.get(), now);
            _reqMap.emplace(_reqId, std::move(req));
            ++_reqId;
        }
        sendPacket(req->getPacket());
    }
}

void RpcClientConn::checkReqTimeout(std::time_t now)
{
    _timeoutMgr->checkTimeout(now);
}

void RpcClientConn::heartbeat()
{
    std::unique_ptr<WsPacket> pkt(new WsPacket());
    pkt->setOpCode(eOpCode::Ping);
    sendPacket(pkt);

    LOG_INFO("RpcClientConn::heartbeat: Sending heartbeat.");
}

void RpcClientConn::onPacket(std::shared_ptr<const RpcSession>&&,
                             std::unique_ptr<WsPacket>&& pkt)
{
    onResponse(std::move(pkt));
}

void RpcClientConn::onClose(RpcClientConn*, std::unique_ptr<WsPacket>&&)
{
    // Empty callback function.
}

void RpcClientConn::onTimeout(RpcRequest* req)
{
    LOG_WARN("RpcClientConn::onTimeout: Request timeout. Request is "
             << req->toString() << ".");
    _reqMap.erase(req->getReqId());
}

void RpcClientConn::onResponse(std::unique_ptr<WsPacket>&& pkt)
{
    uint64_t rpcReqId = 0;
    auto sysJson      = pkt->getSysJson();

    if (!sysJson->containsKey("/rpcReqId") || !sysJson->isUint64("/rpcReqId"))
    {
        LOG_WARN("RpcClientConn::onResponse: Bad sys json: "
                 << sysJson->toString() << ".");
        return;
    }
    sysJson->getValue("/rpcReqId", rpcReqId);

    auto it = _reqMap.find(rpcReqId);
    if (it == _reqMap.end())
    {
        LOG_WARN("RpcClientConn::onPacket: Failed to find reqId: " << rpcReqId
                                                                   << ".");
        return;
    }

    it->second->onResponse(std::move(pkt));
    _timeoutMgr->remove(it->second.get());
    _reqMap.erase(it);
}
}
