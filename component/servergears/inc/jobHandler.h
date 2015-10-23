#ifndef __COMPONENT_SERVERGEAR_INC_JOBHandler_H__
#define __COMPONENT_SERVERGEAR_INC_JOBHandler_H__

#include <mutex>
#include <list>
#include <memory>

#include "job.h"

namespace parrot
{
class JobHandler
{
  public:
    JobHandler() = default;
    virtual ~JobHandler() = default;

  public:
    void addJob(std::unique_ptr<Job>&& job);
    void addJob(std::list<std::unique_ptr<Job>>& jobList);

  protected:
    virtual void handleJob() = 0;

  protected:
    std::mutex _jobListLock;
    std::list<std::unique_ptr<Job>> _jobList;
};
}

#endif
