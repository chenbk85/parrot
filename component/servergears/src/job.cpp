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

void* Job::getDerivedPtr() const
{
    return _derivedPtr;
}

void Job::setDerivedPtr(void* p)
{
    _derivedPtr = p
}
}
