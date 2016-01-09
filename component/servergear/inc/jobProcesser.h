#ifndef __COMPONENT_SERVERGEAR_INC_JOBPROCESSER_H__
#define __COMPONENT_SERVERGEAR_INC_JOBPROCESSER_H__

#include <list>
#include <memory>
#include <unordered_map>

#include "job.h"
#include "jobHandler.h"
#include "threadJob.h"

namespace parrot
{
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
    HdrJobListMap _hdrJobListMap;
};
}

#endif
