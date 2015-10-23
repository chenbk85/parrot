#include "jobHandler.h"

namespace parrot
{
void JobHandler::addJob(std::unique_ptr<ThreadJob>&& job)
{
    _jobListLock.lock();
    _jobList.push_back(std::move(job));
    _jobListLock.unlock();
}

void JobHandler::addJob(std::list<std::unique_ptr<ThreadJob>>& jobList)
{
    _jobListLock.lock();
    _jobList.splice(_jobList.end(), jobList);
    _jobListLock.unlock();
}
}
