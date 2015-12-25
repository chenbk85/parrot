#ifndef __COMPONENT_SERVERGEAR_INC_JOB_H__
#define __COMPONENT_SERVERGEAR_INC_JOB_H__

#include <cstdint>

namespace parrot
{

//\!*/!*\!*/!*\!*/!*\!*/!*\!*/!*\!*/!*\!*/!*\!*/!*\!*/!*\!*/!*
//
// JOB_XXX < 1000 are reserved for system jobs.
//
//\!*/!*\!*/!*\!*/!*\!*/!*\!*/!*\!*/!*\!*/!*\!*/!*\!*/!*\!*/!*
const uint32_t JOB_PACKET         = 1;
const uint32_t JOB_RPC_REQ        = 2;
const uint32_t JOB_RPC_RSP        = 3;
const uint32_t JOB_UPDATE_SESSION = 4;
const uint32_t JOB_DEL_SESSION    = 5;
const uint32_t JOB_KICK           = 6;

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
    void* getDerivedPtr() const;

  protected:
    void setJobType(uint32_t type);
    void setDerivedPtr(void* ptr);

  private:
    void* _derivedPtr;
    uint32_t _jobType;
};
}

#endif
