#ifndef __COMPONENTS_SERVERGEAR_INC_JOB_H__
#define __COMPONENTS_SERVERGEAR_INC_JOB_H__

namespace parrot
{
enum class JobType
{
    None,
    SendPacket,
    ReqBind,
    RsqBind,
    updateSession,
    Kick
};

class Job
{
  public:
    Job(JobType jobType);
    virtual ~Job() = default;
    Job(const Job&) = delete;
    Job& operator=(const Job&) = delete;
    Job(Job&&) = default;
    Job& operator=(Job&&) = default;

  public:
    JobType getJobType() const;
    void setJobType(JobType type);

  private:
    JobType _jobType;
};
}

#endif
