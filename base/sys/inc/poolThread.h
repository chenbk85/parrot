#ifndef __BASE_SYS_INC_POOLTHREAD_H__
#define __BASE_SYS_INC_POOLTHREAD_H__

#include "threadBase.h"

namespace parrot
{
class PoolThread : public ThreadBase
{
  public:
    virtual ~PoolThread() = default;
    
  protected:
    virtual void beforeRun() override;
};
}

#endif
