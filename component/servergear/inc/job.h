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
constexpr uint32_t JOB_PACKET             = 1;

constexpr uint32_t JOB_RPC_SRV_REQ        = 2;
constexpr uint32_t JOB_RPC_SRV_RSP        = 3;

constexpr uint32_t JOB_RPC_CLI_REQ        = 4;
constexpr uint32_t JOB_RPC_CLI_RSP        = 5;

constexpr uint32_t JOB_UPDATE_SESSION     = 6;
constexpr uint32_t JOB_UPDATE_SESSION_ACK = 7;

constexpr uint32_t JOB_KICK               = 8;

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
