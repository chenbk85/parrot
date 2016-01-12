#ifndef __COMPONENT_SERVERGEAR_INC_JOBPROCESSER_H__
#define __COMPONENT_SERVERGEAR_INC_JOBPROCESSER_H__

#include <list>
#include <memory>
#include <unordered_map>

#include "job.h"
#include "threadJob.h"

namespace parrot
{
class JobManager;
class JobProcesser
{
  public:
    JobProcesser() = default;
    virtual ~JobProcesser() = default;

  public:
    void addJob(std::list<std::unique_ptr<Job>>&& jobList);
    void dispatchJobs();

  public:
    virtual void processJobs() = 0;

  protected:
    virtual void loadJobs() = 0;

  protected:
    std::list<std::unique_ptr<Job>> _jobList;
    JobMgrListMap _jobMgrListMap;
};
}

#endif
