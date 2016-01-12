#ifndef __COMPONENT_SERVERGEAR_INC_FRONTTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_FRONTTHREAD_H__

#include <system_error>
#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>
#include <list>
#include <cstdint>
#include <ctime>
#include <iostream>

#include "epoll.h"
#include "kqueue.h"
#include "config.h"
#include "mtRandom.h"
#include "poolThread.h"
#include "threadJob.h"
#include "timeoutHandler.h"
#include "timeoutManager.h"
#include "wsServerConn.h"
#include "eventNotifier.h"
#include "wsPacket.h"
#include "connManager.h"
#include "macroFuncs.h"
#include "logger.h"
#include "scheduler.h"
#include "jobFactory.h"
#include "frontThreadJobProcesser.h"
#include "frontThreadConnManager.h"

namespace parrot
{
template <typename Sess> class FrontThreadJobProcesser;

template <typename Sess>
class FrontThread : public PoolThread,
                    public JobManager,
                    public FrontThreadConnManager<Sess, FrontThread>
{
    using FTConnMgr = FrontThreadConnManager<Sess, FrontThread>;

    friend class FrontThreadJobProcesser<Sess>;

  public:
    FrontThread();

  public:
    void updateByConfig(const Config* cfg);
    const std::string& getServerSid() const;

  public:
    // ThreadBase
    void stop() override;

  protected:
    // ThreadBase
    void run() override;

  private:
    std::unique_ptr<EventNotifier> _notifier;
    std::unique_ptr<FrontThreadJobProcesser<Sess>> _jobProcesser;
    const Config* _config;
};

template <typename Sess>
FrontThread<Sess>::FrontThread()
    : PoolThread(),
      JobManager(),
      FrontThreadConnManager<Sess, FrontThread>(this),
      _notifier(),
      _jobProcesser(),
      _config(nullptr)
{
}

template <typename Sess>
void FrontThread<Sess>::updateByConfig(const Config* cfg)
{
    _config = cfg;

    FTConnMgr::createTimeManager(_config->_frontClientConnTimeout);

    _jobProcesser.reset(new FrontThreadJobProcesser<Sess>(this));

#if defined(__linux__)
    _notifier.reset(new Epoll(_config->_frontThreadMaxConnCount));
#elif defined(__APPLE__)
    _notifier.reset(new Kqueue(_config->_frontThreadMaxConnCount));
#elif defined(_WIN32)
//    _notifier.reset(new
//    SimpleEventNotifier(_config->_frontThreadMaxConnCount));
#endif
    _notifier->create();

    JobManager::setJobProcesser(_jobProcesser.get());
    JobManager::setEventNotifier(_notifier.get());

    FTConnMgr::setJobProcesser(_jobProcesser.get());
    FTConnMgr::setEventNotifier(_notifier.get());
}

template <typename Sess> void FrontThread<Sess>::stop()
{
    ThreadBase::stop();
    _notifier->stopWaiting();
    ThreadBase::join();
    LOG_INFO("FrontThread::stop: Done.");
}

template <typename Sess>
const std::string& FrontThread<Sess>::getServerSid() const
{
    return _config->_thisServer._serverId;
}

template <typename Sess> void FrontThread<Sess>::run()
{
    uint32_t eventNum = 0;
    uint32_t idx      = 0;
    IoEvent* ev       = nullptr;
    eIoAction act     = eIoAction::None;
    std::time_t now   = 0;

    try
    {
        while (!isStopping())
        {
            now = std::time(nullptr);

            FTConnMgr::checkTimeout(now);
            FTConnMgr::addConnToNotifier();

            eventNum = _notifier->waitIoEvents(5000);

            // Here handle events.
            for (idx = 0; idx != eventNum; ++idx)
            {
                // We are sure that the IoEvnet is WsServerConn,
                // so we can use static_cast.
                ev  = _notifier->getIoEvent(idx);
                act = ev->handleIoEvent();
                ev->setNextAction(act);

                switch (act)
                {
                    case eIoAction::Read:
                    case eIoAction::ReadWrite:
                    {
                        if (ev->isConnection())
                        {
                            FTConnMgr::updateTimeout(
                                static_cast<WsServerConn<Sess>*>(
                                    ev->getDerivedPtr()),
                                now);
                        }
                    }
                    // No break;
                    case eIoAction::Write:
                    {
                        _notifier->updateEventAction(ev);
                    }
                    break;

                    case eIoAction::Remove:
                    {
                        FTConnMgr::removeConn(static_cast<WsServerConn<Sess>*>(
                            ev->getDerivedPtr()));
                    }
                    break;

                    default:
                    {
                        PARROT_ASSERT(false);
                    }
                    break;
                } // switch
            }     // for

            handleJobs();
        } // while
    }
    catch (const std::system_error& e)
    {
        LOG_ERROR("FrontThread::run: Errno is " << e.code().message()
                                                << ". Meaning " << e.what());
        // There's nothing we can do here ...
        PARROT_ASSERT(false);
    }
}
}

#endif
