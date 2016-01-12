#ifndef __COMPONENT_SERVERGEAR_INC_SCHEDULER_H__
#define __COMPONENT_SERVERGEAR_INC_SCHEDULER_H__

#include <memory>
#include <cstdint>
#include "jobManager.h"

namespace parrot
{
template <typename Sess> class Scheduler
{
  protected:
    Scheduler() = default;

  public:
    virtual ~Scheduler() = default;

  protected:
    static std::unique_ptr<Scheduler> _scheduler;

  public:
    static Scheduler* getInstance();

    virtual JobManager* getJobManager(uint64_t route,
                                   std::shared_ptr<const Sess>) = 0;
    virtual JobManager* getOnCloseJobManager(std::shared_ptr<const Sess>) = 0;
};

template <typename Sess>
std::unique_ptr<Scheduler<Sess>> Scheduler<Sess>::_scheduler;

template <typename Sess> Scheduler<Sess>* Scheduler<Sess>::getInstance()
{
    return _scheduler.get();
}

}

#endif
