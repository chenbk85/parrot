#ifndef __COMPONENTS_SERVERGEAR_INC_JOBHandler_H__
#define __COMPONENTS_SERVERGEAR_INC_JOBHandler_H__

#include <mutex>
#include <memory>

#inclde "job.h"

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
}
}

#endif
