#ifndef __COMPONENT_SERVERGEAR_INC_RPCCLIENTCONN_H__
#define __COMPONENT_SERVERGEAR_INC_RPCCLIENTCONN_H__

#include <string>
#include <list>
#include <ctime>
#include <unordered_map>
#include <cstdint>

#include "rpcRequest.h"
#include "wsPacket.h"
#include "wsClientConn.h"
#include "timeoutManager.h"
#include "timeoutHandler.h"

namespace parrot
{
struct Config;
struct WsConfig;

class RpcClientConn : public WsClientConn, public TimeoutHandler<RpcRequest>
{
  public:
    RpcClientConn(const Config& cfg,
                  const std::string& wsUrl,
                  const WsConfig& wsCfg);
    RpcClientConn() = default;

  public:
    void addJob(std::unique_ptr<RpcRequest>&& req);
    void addJob(std::list<std::unique_ptr<RpcRequest>>& reqList);
    void checkReqTimeout(std::time_t now);

  private:
    void onTimeout(RpcRequest* req) override;
    void onResponse(std::unique_ptr<WsPacket>&& pkt);

  private:
    std::unique_ptr<TimeoutManager<RpcRequest>> _timeoutMgr;
    std::unordered_map<uint64_t, std::unique_ptr<RpcRequest>> _reqMap;
    uint64_t _reqId;
    const Config& _config;
};
}

#endif
