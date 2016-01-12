#ifndef __PROJECT_CHAT_FRONTSRV_INC_FRONTSRVRPCSCHEDULER_H__
#define __PROJECT_CHAT_FRONTSRV_INC_FRONTSRVRPCSCHEDULER_H__

#include <memory>
#include <cstdint>

#include "jobManager.h"
#include "scheduler.h"
#include "rpcSession.h"

namespace chat
{
class FrontSrvRpcScheduler : public parrot::Scheduler<parrot::RpcSession>
{
  private:
    FrontSrvRpcScheduler() = default;

  public:
    virtual ~FrontSrvRpcScheduler() = default;

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
