#include "job.h"

namespace parrot
{
Job::Job(eJobType jobType) : _jobType(jobType)
{
}

eJobType Job::getJobType() const
{
    return _jobType;
}

void Job::setJobType(eJobType type)
{
    _jobType = type;
}
}
