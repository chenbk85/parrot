#include "job.h"

namespace parrot
{
Job::Job(uint32_t jobType) : _jobType(jobType)
{
}

uint32_t Job::getJobType() const
{
    return _jobType;
}

void Job::setJobType(uint32_t type)
{
    _jobType = type;
}

void* Job::getDerivedPtr() const
{
    return _derivedPtr;
}

void Job::setDerivedPtr(void* p)
{
    _derivedPtr = p;
}
}
