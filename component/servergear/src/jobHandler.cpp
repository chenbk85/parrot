#include "job.h"

namespace parrot
{
void JobHandler::addJob(std::unique_ptr<Job>&& job)
{
    _jobListLock.lock();
    _jobList.push_back(std::move(job));
    _jobListLock.unlock();

    afterAddJob();
}

void JobHandler::addJob(std::list<std::unique_ptr<Job>>& jobList)
{
    _jobListLock.lock();
    _jobList.splice(_jobList.end(), jobList);
    _jobListLock.unlock();

    afterAddJob();
}
}
