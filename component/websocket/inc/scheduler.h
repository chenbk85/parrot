#ifndef __COMPONENT_SERVERGEAR_INC_SCHEDULER_H__
#define __COMPONENT_SERVERGEAR_INC_SCHEDULER_H__

#include <memory>
#include <cstdint>
#include "jobHandler.h"

namespace parrot
{
template <typename Sess> class Scheduler
{
  protected:
    Scheduler() = default;

  public:
    virtual ~Scheduler() = default;

  private:
    static std::unique_ptr<Scheduler> _scheduler;

  public:
    static Scheduler* getInstance();
    static void setInstance(std::unique_ptr<Scheduler>&&);

    virtual JobHandler* getHandler(uint64_t route,
                                   std::shared_ptr<const Sess>) = 0;
    virtual JobHandler* getOnCloseHandler(std::shared_ptr<const Sess>) = 0;
};

template <typename Sess>
std::unique_ptr<Scheduler<Sess>> Scheduler<Sess>::_scheduler;

template <typename Sess> Scheduler<Sess>* Scheduler<Sess>::getInstance()
{
    return _scheduler.get();
}

template <typename Sess>
void Scheduler<Sess>::setInstance(std::unique_ptr<Scheduler>&& p)
{
    _scheduler = std::move(p);
}
}

#endif
