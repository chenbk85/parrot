#ifndef __COMPONENT_SERVERGEAR_INC_SCHEDULER_H__
#define __COMPONENT_SERVERGEAR_INC_SCHEDULER_H__

#include <memory>
#include <cstdint>
#include "jobHandler.h"


namespace parrot
{
template <typename Sess>
class Scheduler
{
  public:
    JobHandler * getHandler(uint64_t route, std::unique_ptr<Sess> &s) = 0;
};
}

#endif
