#ifndef __PROJECT_CHAT_BACKSRV_INC_BACKSRVRPCSCHEDULER_H__
#define __PROJECT_CHAT_BACKSRV_INC_BACKSRVRPCSCHEDULER_H__

#include <memory>
#include <cstdint>

#include "jobManager.h"
#include "scheduler.h"
#include "rpcSession.h"

namespace chat
{
class BackSrvRpcScheduler : public parrot::Scheduler<parrot::RpcSession>
{
  private:
    BackSrvRpcScheduler() = default;

  public:
    virtual ~BackSrvRpcScheduler() = default;

  public:
    static void createInstance();

  public:
    parrot::JobManager*
    getJobManager(uint64_t route,
                  std::shared_ptr<const parrot::RpcSession>) override;

    parrot::JobManager*
    getOnCloseJobManager(std::shared_ptr<const parrot::RpcSession>) override;
};
}

#endif
