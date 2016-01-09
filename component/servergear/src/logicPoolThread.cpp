#include "logger.h"
#include "ioEvent.h"
#include "epoll.h"
#include "kqueue.h"
#include "codes.h"
#include "simpleEventNotifier.h"
#include "macroFuncs.h"
#include "logicPoolThread.h"

namespace parrot
{
LogicPoolThread::LogicPoolThread(uint32_t eventCount)
    : PoolThread(),
      JobHandler(),
#if defined(__linux__)
      _notifier(new Epoll(eventCount))
#elif defined(__APPLE__)
      _notifier(new Kqueue(eventCount))
#elif defined(_WIN32)
//      _notifier(new SimpleEventNotifier())
#endif
{
}

void LogicPoolThread::setJobProcesser(std::unique_ptr<JobProcesser>&& p)
{
    _jobProcesser = std::move(p);
}

void LogicPoolThread::beforeStart()
{
    JobHandler::setJobProcesser(_jobProcesser.get());
    JobHandler::setEventNotifier(_notifier.get());
    _notifier->create();
}

void LogicPoolThread::run()
{
    parrot::IoEvent* ev = nullptr;
    uint32_t ret        = 0;

    LOG_INFO("LogicPoolThread::run. Tid is " << std::this_thread::get_id()
                                             << ".");

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

void LogicPoolThread::stop()
{
    ThreadBase::stop();
    _notifier->stopWaiting();
    ThreadBase::join();
    LOG_INFO("LogicPoolThread::stop: Done.");
}
}
