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
LogicThread::LogicThread(uint32_t eventCount)
    : ThreadBase(),
      JobManager(),
      _jobProcesser(),
#if defined(__linux__)
      _notifier(new Epoll(eventCount))
#elif defined(__APPLE__)
      _notifier(new Kqueue(eventCount))
#elif defined(_WIN32)
//      _notifier(new SimpleEventNotifier())
#endif
{
}

void LogicThread::setJobProcesser(std::unique_ptr<parrot::JobProcesser>&& p)
{
    _jobProcesser = std::move(p);
}

void LogicThread::beforeStart()
{
    JobManager::setJobProcesser(_jobProcesser.get());    
    JobManager::setEventNotifier(_notifier.get());    
    _notifier->create();
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
