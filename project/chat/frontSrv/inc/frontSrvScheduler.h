#ifndef __PROJECT_CHAT_FRONTSRV_INC_FRONTSRVSCHEDULER_H__
#define __PROJECT_CHAT_FRONTSRV_INC_FRONTSRVSCHEDULER_H__

#include <memory>
#include <cstdint>

#include "jobHandler.h"
#include "scheduler.h"
#include "chatSession.h"

namespace chat
{
class FrontSrvScheduler : public parrot::Scheduler<ChatSession>
{
  private:
    FrontSrvScheduler() = default;

  public:
    virtual ~FrontSrvScheduler() = default;

  private:
    static std::unique_ptr<FrontSrvScheduler> _instance;

  public:
    static void makeInstance();

  public:
    parrot::JobHandler* getHandler(uint64_t route,
                                   std::shared_ptr<const ChatSession>) override;

    parrot::JobHandler*
    getOnCloseHandler(std::shared_ptr<const ChatSession>) override;
};
}

#endif
