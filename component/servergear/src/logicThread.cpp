#include "logger.h"
#include "ioEvent.h"
#include "epoll.h"
#include "kqueue.h"
#include "codes.h"
#include "simpleEventNotifier.h"
#include "macroFuncs.h"
#include "logicThread.h"

namespace parrot
{
LogicThread::LogicThread()
    : PoolThread(),
      JobHandler(),
#if defined(__linux__)
      _notifier(new Epoll(1)),
#elif defined(__APPLE__)
      _notifier(new Kqueue(1)),
#elif defined(_WIN32)
//      _notifier(new SimpleEventNotifier()),
#endif
      _jobProcesser()
{
}

void LogicThread::setJobProcesser(std::unique_ptr<JobProcesser>&& p)
{
    _jobProcesser = std::move(p);
}

void LogicThread::beforeStart()
{
    _notifier->create();
}

void LogicThread::afterAddJob()
{
    _notifier->stopWaiting();
}

void LogicThread::handleJobs()
{
    _jobListLock.lock();
    _jobProcesser->addJob(std::move(_jobList));
    _jobList.clear();
    _jobListLock.unlock();

    _jobProcesser->processJobs();
    _jobProcesser->dispatchJobs();
}

void LogicThread::run()
{
    parrot::IoEvent* ev = nullptr;
    uint32_t ret        = 0;

    LOG_INFO("LogicThread::run. Tid is " << std::this_thread::get_id() << ".");

    while (!isStopping())
    {
        ret = _notifier->waitIoEvents(-1);

        for (auto i = 0u; i < ret; ++i)
        {
            ev = _notifier->getIoEvent(i);
            ev->handleIoEvent();
        }

        handleJobs();
    }
}

void LogicThread::stop()
{
    ThreadBase::stop();
    _notifier->stopWaiting();
    ThreadBase::join();
    LOG_INFO("LogicThread::stop: Done.");
}
}
