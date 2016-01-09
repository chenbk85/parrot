#ifndef __COMPONENT_SERVERGEAR_INC_LOGICTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_LOGICTHREAD_H__

#include <memory>
#include <iostream>

#include "poolThread.h"
#include "jobHandler.h"
#include "jobProcesser.h"
#include "eventNotifier.h"

namespace parrot
{
class LogicThread : public PoolThread, public JobHandler
{
  public:
    LogicThread();

  public:
    void setJobProcesser(std::unique_ptr<JobProcesser>&&);

  public:
    void stop() override;
    void afterAddJob() override;

  protected:
    void beforeStart() override;
    void run() override;

  protected:
    void handleJobs() override;

  protected:
    std::unique_ptr<parrot::EventNotifier> _notifier;
    std::unique_ptr<JobProcesser> _jobProcesser;
};
}

#endif
