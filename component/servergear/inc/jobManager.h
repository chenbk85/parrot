#ifndef __COMPONENT_SERVERGEAR_INC_JOBMANAGER_H__
#define __COMPONENT_SERVERGEAR_INC_JOBMANAGER_H__

#include <mutex>
#include <list>
#include <memory>

#include "job.h"

namespace parrot
{
class JobProcesser;
class EventNotifier;

class JobManager
{
  public:
    JobManager();
    virtual ~JobManager() = default;

  public:
    void addJob(std::unique_ptr<Job>&& job);
    void addJob(std::list<std::unique_ptr<Job>>& jobList);

  protected:
    void setJobProcesser(JobProcesser* jp);
    void setEventNotifier(EventNotifier* n);

  protected:
    virtual void handleJobs();

  protected:
    std::mutex _jobListLock;
    std::list<std::unique_ptr<Job>> _jobList;

  private:
    JobProcesser* _baseJobProcesser;
    EventNotifier* _baseNotifier;
};
}

#endif
