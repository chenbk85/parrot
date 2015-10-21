#include "job.h"

namespace parrot
{
Job::Job(JobType jobType) : _jobType(jobType)
{
}

JobType Job::getJobType() const
{
    return _jobType;
}

void Job::setJobType(JobType type)
{
    _jobType = type;
}
}
