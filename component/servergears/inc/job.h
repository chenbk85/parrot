#ifndef __COMPONENT_SERVERGEAR_INC_JOB_H__
#define __COMPONENT_SERVERGEAR_INC_JOB_H__

#include <cstdint>

namespace parrot
{

#define JOB_PACKET 1
#define JOB_REQ_BIND 2
#define JOB_RSP_BIND 3
#define JOB_UPDATE_SESSION 4
#define JOB_DEL_SESSION 5
#define JOB_KICK 6

class Job
{
  public:
    Job(uint32_t jobType);
    virtual ~Job() = default;
    Job(const Job&) = delete;
    Job& operator=(const Job&) = delete;
    Job(Job&&) = default;
    Job& operator=(Job&&) = default;

  public:
    uint32_t getJobType() const;
    void setJobType(uint32_t type);

  private:
    uint32_t _jobType;
};
}

#endif
