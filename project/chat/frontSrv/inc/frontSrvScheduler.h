#ifndef __PROJECT_CHAT_FRONTSRV_INC_FRONTSRVSCHEDULER_H__
#define __PROJECT_CHAT_FRONTSRV_INC_FRONTSRVSCHEDULER_H__

#include <memory>
#include <cstdint>

#include "jobManager.h"
#include "scheduler.h"
#include "chatSession.h"

namespace chat
{
struct FrontSrvConfig;

class FrontSrvScheduler : public parrot::Scheduler<ChatSession>
{
  private:
    FrontSrvScheduler() = default;

  public:
    virtual ~FrontSrvScheduler() = default;

  public:
    static void createInstance();

  public:
    parrot::JobManager*
    getJobManager(uint64_t route, std::shared_ptr<const ChatSession>) override;

    parrot::JobManager*
    getOnCloseJobManager(std::shared_ptr<const ChatSession>) override;
};
}

#endif
