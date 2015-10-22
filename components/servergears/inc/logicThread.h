#ifndef __COMPONENTS_SERVERGEAR_INC_LOGICTHREAD_H__
#define __COMPONENTS_SERVERGEAR_INC_LOGICTHREAD_H__

#include <memory>
#include <mutex>
#include <list>
#include <cstdint>

#include "poolThread.h"

namespace parrot
{
struct Config;
class EventNotifier;
    
class LogicThread : public PoolThread
{
  public:
    LogicThread();

  public:
    
}
}

#endif
