#ifndef __COMPONENT_SERVERGEAR_INC_RPCCLIENTTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_RPCCLIENTTHREAD_H__

#include <memory>
#include <list>

#include "threadBase.h"
#include "mtRandom.h"
#include "jobHandler.h"
#include "threadJob.h"
#include "timeoutHandler.h"
#include "timeoutManager.h"
#include "wsClientConn.h"
#include "eventNotifier.h"
#include "wsPacket.h"
#include "connHandler.h"

namespace parrot
{
class RpcClientThread : public ThreadBase,
                        public TimeoutHandler<RpcClientConn>,
                        public JobHandler
{
  public:
    RpcClientThread(const Config& cfg, const WsConfig& wsCfg);

  public:
    void addJob(std::unique_ptr<Job>&& job);
    void addJob(std::list<std::unique_ptr<Job>>& jobList);

  private:
    void init();    
    void doConnect();
    
  private:
    void onTimeout() override;
    void beforeStart() override;
    void run() override;

  private:
    std::mutex _jobListLock;
    std::unordered_map<std::string, std::unique_ptr<job>> _jobList;

    std::list<std::unique_ptr<RpcClientConn>> _disconnectedConnList;

    // Save connected client to this map.
    std::unordered_map<std::string, std::shared_ptr<RpcClientConn>> _connMap;

    std::unique_ptr<EventNotifier> _notifier;
    std::unique_ptr<TimeoutManager<RpcClientConn<RpcSession>>> _timeoutMgr;    
    MtRandom _random;
    const Config& _config;
    const WsConfig& _wsConfig;
};
}

#endif
