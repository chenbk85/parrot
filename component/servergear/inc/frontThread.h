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
#include "jobManager.h"
#include "threadJob.h"
#include "timeoutHandler.h"
#include "timeoutManager.h"
#include "wsServerConn.h"
#include "eventNotifier.h"
#include "wsPacket.h"
#include "connHandler.h"
#include "macroFuncs.h"
#include "logger.h"
#include "scheduler.h"
#include "jobFactory.h"
#include "frontThreadJobProcesser.h"

namespace parrot
{
template <typename Sess> class FrontThreadJobProcesser;

template <typename Sess>
class FrontThread : public PoolThread,
                    public TimeoutHandler<WsServerConn<Sess>>,
                    public JobManager,
                    public ConnHandler<WsServerConn<Sess>>,
                    public WsPacketHandler<Sess, WsServerConn<Sess>>
{
    using ConnMap =
        std::unordered_map<std::string, std::unique_ptr<WsServerConn<Sess>>>;

    friend class FrontThreadJobProcesser<Sess>;

  public:
    FrontThread();

  public:
    void updateByConfig(const Config* cfg);

  public:
    // ThreadBase
    void stop() override;

  protected:
    // ThreadBase
    void run() override;

    // TimeoutHandler
    void onTimeout(WsServerConn<Sess>*) override;

  public:
    // WsPacketHandler
    void onPacket(WsServerConn<Sess>* conn,
                  std::unique_ptr<WsPacket>&&) override;
    void onClose(WsServerConn<Sess>* conn,
                 std::unique_ptr<WsPacket>&&) override;

  protected:
    void addConnToNotifier();
    void removeConn(WsServerConn<Sess>* conn);
    void updateTimeout(WsServerConn<Sess>* conn, std::time_t now);

  private:
    ConnMap _connMap;

    std::unique_ptr<EventNotifier> _notifier;
    std::unique_ptr<FrontThreadJobProcesser<Sess>> _jobProcesser;

    MtRandom _random;
    std::unique_ptr<TimeoutManager<WsServerConn<Sess>>> _timeoutMgr;
    uint32_t _connUniqueIdx;
    const Config* _config;
};

template <typename Sess>
FrontThread<Sess>::FrontThread()
    : PoolThread(),
      TimeoutHandler<WsServerConn<Sess>>(),
      JobManager(),
      ConnHandler<WsServerConn<Sess>>(),
      WsPacketHandler<Sess, WsServerConn<Sess>>(),
      _connMap(),
      _notifier(nullptr),
      _jobProcesser(),
      _random(),
      _timeoutMgr(),
      _connUniqueIdx(0),
      _config(nullptr)
{
}

template <typename Sess>
void FrontThread<Sess>::updateByConfig(const Config* cfg)
{
    _config = cfg;

    _timeoutMgr.reset(new TimeoutManager<WsServerConn<Sess>>(
        this, _config->_frontClientConnTimeout));

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

    setJobProcesser(_jobProcesser.get());
    JobManager::setEventNotifier(_notifier.get());
    ConnHandler<WsServerConn<Sess>>::setEventNotifier(_notifier.get());
}

template <typename Sess> void FrontThread<Sess>::stop()
{
    ThreadBase::stop();
    _notifier->stopWaiting();
    ThreadBase::join();
    LOG_INFO("FrontThread::stop: Done.");
}

template <typename Sess>
void FrontThread<Sess>::onPacket(WsServerConn<Sess>* conn,
                                 std::unique_ptr<WsPacket>&& pkt)
{
    // A copy of shard_ptr. Need to move next.
    auto session = conn->getSession();
    auto route   = pkt->getRoute();
    auto mgr = session->getRouteJobMgr(route);
    if (!mgr)
    {
        mgr = Scheduler<Sess>::getInstance()->getJobManager(route, session);
        if (!mgr)
        {
            LOG_WARN("FrontThread::onPacket: Failed to find job manager "
                     "for session "
                     << session->toString() << ". Route is " << route << ".");
            return;
        }
        session->setRouteJobMgr(route, mgr);
    }

    if (!mgr)
    {
        LOG_WARN(
            "FrontThread::onPacket: Failed to find job manager for session "
            << session->toString() << ". Route is " << pkt->getRoute() << ".");
        return;
    }
    _jobProcesser->createPacketJobs(
        mgr, PacketJobParam<Sess>(std::move(session), std::move(pkt)));
}

template <typename Sess>
void FrontThread<Sess>::onClose(WsServerConn<Sess>* conn,
                                std::unique_ptr<WsPacket>&& pkt)
{
    auto session = conn->getSession(); // A copy of shared_ptr.
    auto mgr = Scheduler<Sess>::getInstance()->getOnCloseJobManager(session);
    if (!mgr)
    {
        LOG_WARN("FrontThread::onClose: Failed to find mgr for session "
                 << session->toString() << ".");
        removeConn(conn);
        return;
    }

    _jobProcesser->createPacketJobs(
        mgr, PacketJobParam<Sess>(std::move(session), std::move(pkt)));

    removeConn(conn);
    LOG_INFO("FrontThread::onClose: Err is " << (uint32_t)(pkt->getCloseCode())
                                             << ". Sess is "
                                             << session->toString() << ".");
}

template <typename Sess> void FrontThread<Sess>::addConnToNotifier()
{
    std::list<std::unique_ptr<WsServerConn<Sess>>> tmpList;

    ConnHandler<WsServerConn<Sess>>::_newConnListLock.lock();
    tmpList = std::move(ConnHandler<WsServerConn<Sess>>::_newConnList);
    ConnHandler<WsServerConn<Sess>>::_newConnListLock.unlock();

    auto now = std::time(nullptr);

    for (auto& c : tmpList)
    {
        std::shared_ptr<Sess> sess(new Sess());
        sess->createUniqueSessionId(_config->_thisServer._serverId,
                                    getThreadIdStr(), _connUniqueIdx++);
        sess->setIpAddrPort(c->getRemoteAddr(), c->getRemotePort());
        sess->setFrontJobMgr(this);

        c->setSession(std::move(sess));
        c->setNextAction(c->getDefaultAction());
        c->setPacketHandler(this);
        c->setRandom(&_random);

        _timeoutMgr->add(c.get(), now);
        _notifier->addEvent(c.get());

        LOG_DEBUG("FrontThread::addConnToNotifier: Add client connection "
                  << c->getRemoteAddr() << ".");
        _connMap[c->getSession()->getUniqueSessionId()] = std::move(c);
    }
}

template <typename Sess>
void FrontThread<Sess>::removeConn(WsServerConn<Sess>* conn)
{
    LOG_DEBUG("FrontThread::removeConn: Client " << conn->getRemoteAddr()
                                                 << " disconnected.");
    _timeoutMgr->remove(conn);
    _notifier->delEvent(conn);
    _connMap.erase(conn->getSession()->getUniqueSessionId());
}

template <typename Sess>
void FrontThread<Sess>::updateTimeout(WsServerConn<Sess>* conn, std::time_t now)
{
    _timeoutMgr->update(conn, now);
}

template <typename Sess>
void FrontThread<Sess>::onTimeout(WsServerConn<Sess>* conn)
{
    std::unique_ptr<WsPacket> pkt(new WsPacket());
    pkt->setOpCode(eOpCode::Close);
    pkt->setClose(eCodes::WS_NormalClosure);
    onClose(conn, std::move(pkt));
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
            _timeoutMgr->checkTimeout(now);

            addConnToNotifier();

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
                            updateTimeout(static_cast<WsServerConn<Sess>*>(
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
                        removeConn(static_cast<WsServerConn<Sess>*>(
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
