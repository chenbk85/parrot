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
#include "jobFactory.h"
#include "rpcServerJobProcesser.h"

namespace parrot
{
struct Config;
class RpcServerJobProcesser;

class RpcServerThread : public ThreadBase,
                        public TimeoutHandler<WsServerConn<RpcSession>>,
                        public JobHandler,
                        public ConnHandler<RpcServerConn>
{
    friend class RpcServerJobProcesser;

    using ConnMap =
        std::unordered_map<RpcServerConn*, std::unique_ptr<RpcServerConn>>;

  public:
    explicit RpcServerThread(const Config* cfg);
    virtual ~RpcServerThread() = default;

  public:
    // ThreadBase
    void stop() override;

    void registerConn(const std::string& sid, RpcServerConn* conn);
    void removeConn(RpcServerConn* conn);

    void addReqPacket(JobHandler* hdr, RpcSrvReqJobParam&& jobParam);

  protected:
    void afterAddNewConn() override;

    void afterAddJob() override;

    // ThreadBase
    void run() override;

    // TimeoutHandler
    void onTimeout(WsServerConn<RpcSession>*) override;

    // JobHandler
    void handleJobs() override;

  private:
    void init();
    void addConnToNotifier();
    void updateTimeout(RpcServerConn* conn, std::time_t now);

  private:
    ConnMap _connMap;
    // <Remote sid, RpcServerConn>
    std::unordered_map<std::string, RpcServerConn*> _registeredConnMap;

    std::unique_ptr<RpcServerJobProcesser> _jobProcesser;

    std::unique_ptr<EventNotifier> _notifier;
    std::unique_ptr<TimeoutManager<WsServerConn<RpcSession>>> _timeoutMgr;

    std::time_t _now;

    MtRandom _random;

    const Config* _config;
};
}

#endif
