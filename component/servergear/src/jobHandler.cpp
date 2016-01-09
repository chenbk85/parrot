#include "jobHandler.h"
#include "job.h"
#include "jobProcesser.h"
#include "eventNotifier.h"

namespace parrot
{

JobHandler::JobHandler()
    : _jobListLock(),
      _jobList(),
      _baseJobProcesser(nullptr),
      _baseNotifier(nullptr)
{
}

void JobHandler::setJobProcesser(JobProcesser* jp)
{
    _baseJobProcesser = jp;
}

void JobHandler::setEventNotifier(EventNotifier* n)
{
    _baseNotifier = n;
}

void JobHandler::addJob(std::unique_ptr<Job>&& job)
{
    _jobListLock.lock();
    _jobList.push_back(std::move(job));
    _jobListLock.unlock();

    _baseNotifier->stopWaiting();
}

void JobHandler::addJob(std::list<std::unique_ptr<Job>>& jobList)
{
    _jobListLock.lock();
    _jobList.splice(_jobList.end(), jobList);
    _jobListLock.unlock();

    _baseNotifier->stopWaiting();
}

void JobHandler::handleJobs()
{
    _jobListLock.lock();
    _baseJobProcesser->addJob(std::move(_jobList));
    _jobListLock.unlock();

    _baseJobProcesser->processJobs();
    _baseJobProcesser->dispatchJobs();
}
}
