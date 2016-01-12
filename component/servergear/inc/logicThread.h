#ifndef __COMPONENT_SERVERGEAR_INC_LOGICTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_LOGICTHREAD_H__

#include <memory>
#include <iostream>
#include <cstdint>

#include "threadBase.h"
#include "jobManager.h"
#include "jobProcesser.h"
#include "eventNotifier.h"

namespace parrot
{
class LogicThread : public ThreadBase, public JobManager
{
  public:
    LogicThread(uint32_t eventCount = 1);

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
