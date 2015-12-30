#ifndef __PROJECT_CHAT_FRONTSRV_INC_FRONTSRVRPCSCHEDULER_H__
#define __PROJECT_CHAT_FRONTSRV_INC_FRONTSRVRPCSCHEDULER_H__

#include <memory>
#include <cstdint>

#include "jobHandler.h"
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

  private:
    static std::unique_ptr<FrontSrvRpcScheduler> _instance;

  public:
    static void makeInstance();

  public:
    parrot::JobHandler*
    getHandler(uint64_t route,
               std::shared_ptr<const parrot::RpcSession>) override;

    parrot::JobHandler*
    getOnCloseHandler(std::shared_ptr<const parrot::RpcSession>) override;
};
}

#endif
