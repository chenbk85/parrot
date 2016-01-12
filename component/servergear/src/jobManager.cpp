#include "jobManager.h"
#include "job.h"
#include "jobProcesser.h"
#include "eventNotifier.h"

namespace parrot
{
JobManager::JobManager()
    : _jobListLock(),
      _jobList(),
      _baseJobProcesser(nullptr),
      _baseNotifier(nullptr)
{
}

void JobManager::setJobProcesser(JobProcesser* jp)
{
    _baseJobProcesser = jp;
}

void JobManager::setEventNotifier(EventNotifier* n)
{
    _baseNotifier = n;
}

void JobManager::addJob(std::unique_ptr<Job>&& job)
{
    _jobListLock.lock();
    _jobList.push_back(std::move(job));
    _jobListLock.unlock();

    _baseNotifier->stopWaiting();
}

void JobManager::addJob(std::list<std::unique_ptr<Job>>& jobList)
{
    _jobListLock.lock();
    _jobList.splice(_jobList.end(), jobList);
    _jobListLock.unlock();

    _baseNotifier->stopWaiting();
}

void JobManager::handleJobs()
{
    _jobListLock.lock();
    _baseJobProcesser->addJob(std::move(_jobList));
    _jobListLock.unlock();

    _baseJobProcesser->processJobs();
    _baseJobProcesser->dispatchJobs();
}
}
