#ifndef __COMPONENT_SERVERGEAR_INC_RPCCLIENTCONN_H__
#define __COMPONENT_SERVERGEAR_INC_RPCCLIENTCONN_H__

#include <string>
#include <list>
#include <ctime>
#include <memory>
#include <unordered_map>
#include <cstdint>

#include "codes.h"
#include "logger.h"
#include "rpcRequest.h"
#include "rpcSession.h"
#include "wsPacket.h"
#include "wsClientConn.h"
#include "timeoutManager.h"
#include "timeoutHandler.h"
#include "config.h"
#include "wsConfig.h"

namespace parrot
{
struct Config;
struct WsConfig;

template <typename Sess> class RpcClientThread;

template <typename Sess>
class RpcClientConn
    : public WsClientConn<RpcSession>,
      public TimeoutHandler<RpcRequest<Sess>>,
      public WsPacketHandler<RpcSession, WsClientConn<RpcSession>>
{
  public:
    RpcClientConn(RpcClientThread<Sess>* thread,
                  const Config& cfg,
                  const std::string& wsUrl,
                  const WsConfig& wsCfg);

  public:
    void addJob(std::unique_ptr<RpcRequest<Sess>>& req);
    void checkReqTimeout(std::time_t now);
    void heartbeat();

  private:
    void onPacket(WsClientConn<RpcSession>* conn,
                  std::unique_ptr<WsPacket>&&) override;
    void onClose(WsClientConn<RpcSession>* conn,
                 std::unique_ptr<WsPacket>&&) override;
    void onTimeout(RpcRequest<Sess>* req) override;
    void onResponse(std::unique_ptr<WsPacket>&& pkt);

  private:
    RpcClientThread<Sess>* _rpcClientThread;
    std::unique_ptr<TimeoutManager<RpcRequest<Sess>>> _timeoutMgr;
    std::unordered_map<uint64_t, std::unique_ptr<RpcRequest<Sess>>> _reqMap;
    uint64_t _reqId;
    const Config& _config;
};

template <typename Sess>
RpcClientConn<Sess>::RpcClientConn(RpcClientThread<Sess>* thread,
                                   const Config& cfg,
                                   const std::string& wsUrl,
                                   const WsConfig& wscfg)
    : WsClientConn<RpcSession>(wsUrl, wscfg, false),
      TimeoutHandler<RpcRequest<Sess>>(),
      WsPacketHandler<RpcSession, WsClientConn<RpcSession>>(),
      _rpcClientThread(thread),
      _timeoutMgr(
          new TimeoutManager<RpcRequest<Sess>>(this, cfg._rpcReqTimeout)),
      _reqMap(),
      _reqId(0),
      _config(cfg)
{
    setPacketHandler(this);
}

template <typename Sess>
void RpcClientConn<Sess>::addJob(std::unique_ptr<RpcRequest<Sess>>& req)
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

template <typename Sess>
void RpcClientConn<Sess>::checkReqTimeout(std::time_t now)
{
    _timeoutMgr->checkTimeout(now);
}

template <typename Sess> void RpcClientConn<Sess>::heartbeat()
{
    std::unique_ptr<WsPacket> pkt(new WsPacket());
    pkt->setOpCode(eOpCode::Ping);
    sendPacket(pkt);

    LOG_INFO("RpcClientConn::heartbeat: Sending heartbeat.");
}

template <typename Sess>
void RpcClientConn<Sess>::onPacket(WsClientConn<RpcSession>*,
                                   std::unique_ptr<WsPacket>&& pkt)
{
    onResponse(std::move(pkt));
}

template <typename Sess>
void RpcClientConn<Sess>::onClose(WsClientConn<RpcSession>*,
                                  std::unique_ptr<WsPacket>&&)
{
    // Empty callback function.
}

template <typename Sess>
void RpcClientConn<Sess>::onTimeout(RpcRequest<Sess>* req)
{
    LOG_WARN("RpcClientConn::onTimeout: Request timeout. Request is "
             << req->toString() << ".");

    _reqMap.erase(req->getReqId());
    _rpcClientThread->addRsp(req->getRspHandler(), eCodes::ERR_Timeout,
                             req->getSession(),
                             std::unique_ptr<WsPacket>(new WsPacket()));
}

template <typename Sess>
void RpcClientConn<Sess>::onResponse(std::unique_ptr<WsPacket>&& pkt)
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
        LOG_WARN("RpcClientConn::onResponse: Failed to find reqId: " << rpcReqId
                                                                     << ".");
        return;
    }

    LOG_INFO("RpcClientConn::onResponse: Received response for rpc request: "
             << it->second->toString() << ".");

    _rpcClientThread->addRsp(it->second->getRspHandler(), eCodes::ST_Ok,
                             it->second->getSession(), std::move(pkt));
    _timeoutMgr->remove(it->second.get());
    _reqMap.erase(it);
}
}

#endif
