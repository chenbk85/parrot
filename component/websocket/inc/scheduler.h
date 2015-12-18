#ifndef __COMPONENT_SERVERGEAR_INC_SCHEDULER_H__
#define __COMPONENT_SERVERGEAR_INC_SCHEDULER_H__

#include <memory>
#include <cstdint>
#include "jobHandler.h"

namespace parrot
{
template <typename Sess> class Scheduler
{
  private:
    virutal ~Scheduler() = default;

  public:
    static Scheduler* getInstance();
    virtual JobHandler* getHandler(uint64_t route,
                                   std::shared_ptr<const Sess>&) = 0;
    virtual JobHandler* getOnCloseHandler(std::shared_ptr<const Sess>&) = 0;
};
}

#endif
