#ifndef __COMPONENT_SERVERGEAR_INC_RPCSERVERTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_RPCSERVERTHREAD_H__

#include <system_error>
#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>
#include <list>
#include <functional>
#include <cstdint>
#include <ctime>
#include <iostream>

#include "config.h"
#include "mtRandom.h"
#include "threadBase.h"
#include "jobHandler.h"
#include "threadJob.h"
#include "timeoutHandler.h"
#include "timeoutManager.h"
#include "rpcServerConn.h"
#include "eventNotifier.h"
#include "wsPacket.h"
#include "rpcSession.h"
#include "connHandler.h"
#include "scheduler.h"
#include "wsServerConn.h"

namespace parrot
{
class RpcServerThread : public ThreadBase,
                        public TimeoutHandler<WsServerConn<RpcSession>>,
                        public JobHandler,
                        public ConnHandler<RpcServerConn>
{
    using RspPktList = std::list<
        std::pair<std::shared_ptr<RpcSession>, std::unique_ptr<WsPacket>>>;

    using ReqPktList = std::list<std::tuple<std::shared_ptr<RpcSession>,
                                            std::unique_ptr<Json>,
                                            std::unique_ptr<WsPacket>>>;

    using ConnMap =
        std::unordered_map<RpcServerConn*, std::unique_ptr<RpcServerConn>>;

  public:
    RpcServerThread();

  public:
    // ThreadBase
    void stop() override;

    void registerConn(const std::string& sid, RpcServerConn* conn);
    void removeConn(RpcServerConn* conn);    

    void addReqPacket(JobHandler* hdr,
                      std::shared_ptr<RpcSession>,
                      std::unique_ptr<Json>&& cliSession,
                      std::unique_ptr<WsPacket>&& pkt);

  protected:
    void afterAddNewConn() override;

    void afterAddJob() override;

    // ThreadBase
    void run() override;

    // TimeoutHandler
    void onTimeout(WsServerConn<RpcSession>*) override;

    // JobHandler
    void handleJob() override;

  private:
    void handleRpcRsp(RspPktList& pktList);
    void dispatchPackets();
    void addConnToNotifier();
    void updateTimeout(RpcServerConn* conn, std::time_t now);

  private:
    ConnMap _connMap;
    // <Remote sid, RpcServerConn>
    std::unordered_map<std::string, RpcServerConn*> _registeredConnMap;

    std::unordered_map<JobHandler*, ReqPktList> _reqMap;

    std::unique_ptr<EventNotifier> _notifier;
    std::unique_ptr<TimeoutManager<WsServerConn<RpcSession>>> _timeoutMgr;

    std::time_t _now;

    MtRandom _random;
    RpcResponseJobHdr _rpcRspJobHdr;
};
}

#endif
