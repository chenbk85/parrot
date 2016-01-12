#ifndef __COMPONENT_SERVERGEAR_INC_LOGICPOOLTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_LOGICPOOLTHREAD_H__

#include <memory>
#include <iostream>
#include <cstdint>

#include "poolThread.h"
#include "jobManager.h"
#include "jobProcesser.h"
#include "eventNotifier.h"

namespace parrot
{
class LogicPoolThread : public PoolThread, public JobManager
{
  public:
    LogicPoolThread(uint32_t eventCount = 1);
    
  public:
    void setJobProcesser(std::unique_ptr<JobProcesser>&& p);
    
  public:
    void stop() override;

  protected:
    void beforeStart() override;
    void run() override;

  protected:
    std::unique_ptr<JobProcesser> _jobProcesser;    
    std::unique_ptr<EventNotifier> _notifier;
};
}

#endif
