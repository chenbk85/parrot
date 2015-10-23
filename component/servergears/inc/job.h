#ifndef __COMPONENT_SERVERGEAR_INC_JOB_H__
#define __COMPONENT_SERVERGEAR_INC_JOB_H__

namespace parrot
{
enum class eJobType
{
    None,
    Packet,
    ReqBind,
    RsqBind,
    UpdateSession,
    DisconnectSession,
    Kick
};

class Job
{
  public:
    Job(eJobType jobType);
    virtual ~Job() = default;
    Job(const Job&) = delete;
    Job& operator=(const Job&) = delete;
    Job(Job&&) = default;
    Job& operator=(Job&&) = default;

  public:
    eJobType getJobType() const;
    void setJobType(eJobType type);

  private:
    eJobType _jobType;
};
}

#endif
