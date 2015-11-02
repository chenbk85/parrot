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
    virtual void addJob(std::unique_ptr<Job>&& job) = 0;
    virtual void addJob(std::list<std::unique_ptr<Job>>& jobList) = 0;

  protected:
    virtual void handleJob() = 0;
};
}

#endif
