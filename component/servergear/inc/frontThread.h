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
#include "jobHandler.h"
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

namespace parrot
{
template <typename Sess>
class FrontThread : public PoolThread,
                    public TimeoutHandler<WsServerConn<Sess>>,
                    public JobHandler,
                    public ConnHandler<WsServerConn<Sess>>,
                    public WsPacketHandler<Sess, WsServerConn<Sess>>
{
    enum class Constants
    {
        kPktListSize = 100
    };

    using PktMap =
        std::unordered_map<JobHandler*, std::list<SessionPktPair<Sess>>>;
    using ConnMap =
        std::unordered_map<std::string, std::unique_ptr<WsServerConn<Sess>>>;

  public:
    FrontThread();

  public:
    void updateByConfig(const Config* cfg);

  public:
    // ThreadBase
    void stop() override;

  protected:
    void afterAddNewConn() override;
    void afterAddJob() override;

    // ThreadBase
    void run() override;

    // TimeoutHandler
    void onTimeout(WsServerConn<Sess>*) override;

    // JobHandler
    void handleJob() override;

  public:
    // WsPacketHandler
    void onPacket(WsServerConn<Sess>* conn,
                  std::unique_ptr<WsPacket>&&) override;
    void onClose(WsServerConn<Sess>* conn,
                 std::unique_ptr<WsPacket>&&) override;

  protected:
    void handleUpdateSession(std::shared_ptr<const Sess>& ps);
    void handlePacket(std::list<SessionPktPair<Sess>>& pktList);
    void dispatchPackets();
    void addConnToNotifier();
    void removeConn(WsServerConn<Sess>* conn);
    void updateTimeout(WsServerConn<Sess>* conn, std::time_t now);

  private:
    PktMap _pktMap;
    ConnMap _connMap;

    std::unique_ptr<EventNotifier> _notifier;

    UpdateSessionJobHdr<Sess> _updateSessionHdr;
    PacketJobHdr<Sess> _pktJobHdr;

    MtRandom _random;
    std::unique_ptr<TimeoutManager<WsServerConn<Sess>>> _timeoutMgr;
    uint32_t _connUniqueIdx;
    const Config* _config;
};

template <typename Sess>
FrontThread<Sess>::FrontThread()
    : PoolThread(),
      TimeoutHandler<WsServerConn<Sess>>(),
      JobHandler(),
      ConnHandler<WsServerConn<Sess>>(),
      WsPacketHandler<Sess, WsServerConn<Sess>>(),
      _pktMap(),
      _connMap(),
      _notifier(nullptr),
      _updateSessionHdr(),
      _pktJobHdr(),
      _random(),
      _timeoutMgr(),
      _connUniqueIdx(0),
      _config(nullptr)
{
    using namespace std::placeholders;

    _updateSessionHdr = std::bind(&FrontThread::handleUpdateSession, this, _1);
    _pktJobHdr        = std::bind(&FrontThread::handlePacket, this, _1);
}

template <typename Sess>
void FrontThread<Sess>::updateByConfig(const Config* cfg)
{
    _config = cfg;
    _timeoutMgr.reset(new TimeoutManager<WsServerConn<Sess>>(
        this, _config->_frontThreadTimeout));

#if defined(__linux__)
    _notifier.reset(new Epoll(_config->_frontThreadMaxConnCount));
#elif defined(__APPLE__)
    _notifier.reset(new Kqueue(_config->_frontThreadMaxConnCount));
#elif defined(_WIN32)
//    _notifier.reset(new
//    SimpleEventNotifier(_config->_frontThreadMaxConnCount));
#endif
    _notifier->create();
}

template <typename Sess> void FrontThread<Sess>::stop()
{
    ThreadBase::stop();
    _notifier->stopWaiting();
    ThreadBase::join();
    LOG_INFO("FrontThread::Stop: Done.");
}

template <typename Sess>
void FrontThread<Sess>::handleUpdateSession(std::shared_ptr<const Sess>& ps)
{
    auto it = _connMap.find(ps->getUniqueSessionId());

    if (it == _connMap.end())
    {
        LOG_WARN("FrontThread::handleUpdateSession: Failed to bind conn key "
                 << ps->getUniqueSessionId() << ". Sess is " << ps->toString()
                 << ".");
        return;
    }

    it->second->updateSession(ps);
}

template <typename Sess>
void FrontThread<Sess>::handlePacket(std::list<SessionPktPair<Sess>>& pktList)
{
    typename ConnMap::iterator it;
    for (auto& s : pktList)
    {
        it = _connMap.find((s.first)->getUniqueSessionId());
        if (it == _connMap.end())
        {
            LOG_DEBUG("FrontThread::handlePacket: Failed to find session "
                      << (s.first)->toString() << ".");
            continue;
        }

        LOG_DEBUG("FrontThread::handlePacket: Send packet to session "
                  << (s.first)->toString());
        it->second->sendPacket(s.second);

        if (it->second->canSwitchToSend())
        {
            it->second->setNextAction(eIoAction::Write);
            _notifier->updateEventAction(it->second.get());
        }
    }
}

template <typename Sess> void FrontThread<Sess>::handleJob()
{
    std::list<std::unique_ptr<Job>> jobList;
    _jobListLock.lock();
    jobList = std::move(_jobList);
    _jobListLock.unlock();

    for (auto& j : jobList)
    {
        switch (j->getJobType())
        {
            case JOB_UPDATE_SESSION:
            {
                std::unique_ptr<UpdateSessionJob<Sess>> tj(
                    static_cast<UpdateSessionJob<Sess>*>(
                        (j.release())->getDerivedPtr()));
                tj->call(_updateSessionHdr);
            }
            break;

            case JOB_PACKET:
            {
                std::unique_ptr<PacketJob<Sess>> tj(
                    static_cast<PacketJob<Sess>*>(
                        (j.release())->getDerivedPtr()));
                tj->call(_pktJobHdr);
            }
            break;

            case JOB_KICK:
            {
            }
            break;

            default:
            {
                PARROT_ASSERT(false);
            }
            break;
        }
    }
}

template <typename Sess> void FrontThread<Sess>::afterAddJob()
{
    _notifier->stopWaiting();
}

template <typename Sess> void FrontThread<Sess>::afterAddNewConn()
{
    _notifier->stopWaiting();
}

template <typename Sess>
void FrontThread<Sess>::onPacket(WsServerConn<Sess>* conn,
                                 std::unique_ptr<WsPacket>&& pkt)
{
    // A copy of shard_ptr. Need to move next.
    auto session = conn->getSession();
    auto route   = pkt->getRoute();
    auto hdr = session->getRouteHdr(route);
    if (!hdr)
    {
        hdr = Scheduler<Sess>::getInstance()->getHandler(route, session);
        if (!hdr)
        {
            LOG_WARN("FrontThread::onPacket: Failed to find handler "
                     "for session "
                     << session->toString() << ". Route is " << route << ".");
            return;
        }
        session->setRouteHdr(route, hdr);
    }

    if (!hdr)
    {
        LOG_WARN("FrontThread::onPacket: Failed to find handler for session "
                 << session->toString() << ". Route is " << pkt->getRoute()
                 << ".");
        return;
    }

    auto it = _pktMap.find(hdr);
    if (it == _pktMap.end())
    {
        PARROT_ASSERT(hdr);
        _pktMap[hdr].emplace_back(std::move(session), std::move(pkt));
    }
    else
    {
        LOG_DEBUG("FrontThread::onPacket: Append packet.");
        it->second.emplace_back(std::move(session), std::move(pkt));
        if (it->second.size() >= static_cast<uint32_t>(Constants::kPktListSize))
        {
            // Dispatch packetes.
            std::unique_ptr<PacketJob<Sess>> pktJob(new PacketJob<Sess>());
            pktJob->bind(std::move(it->second));
            hdr->addJob(std::move(pktJob));
            it->second.clear();
        }
    }
}

template <typename Sess>
void FrontThread<Sess>::onClose(WsServerConn<Sess>* conn,
                                std::unique_ptr<WsPacket>&& pkt)
{
    auto session = conn->getSession(); // A copy of shared_ptr.
    auto hdr = Scheduler<Sess>::getInstance()->getOnCloseHandler(session);
    if (!hdr)
    {
        LOG_WARN("FrontThread::onClose: Failed to find hdr for session "
                 << session->toString() << ".");
        removeConn(conn);
        return;
    }

    auto it = _pktMap.find(hdr);
    if (it == _pktMap.end())
    {
        _pktMap[hdr].emplace_back(std::move(session), std::move(pkt));
    }
    else
    {
        it->second.emplace_back(std::move(session), std::move(pkt));
        if (it->second.size() >= static_cast<uint32_t>(Constants::kPktListSize))
        {
            // Dispatch packetes.
            std::unique_ptr<PacketJob<Sess>> pktJob(new PacketJob<Sess>());
            pktJob->bind(std::move(it->second));
            hdr->addJob(std::move(pktJob));
            it->second.clear();
        }
    }

    removeConn(conn);
    LOG_INFO("FrontThread::onClose: Err is " << (uint32_t)(pkt->getCloseCode())
                                             << ". Sess is "
                                             << session->toString() << ".");
}

template <typename Sess> void FrontThread<Sess>::dispatchPackets()
{
    for (auto& kv : _pktMap)
    {
        if (!kv.second.empty())
        {
            LOG_DEBUG("FrontThread::dispatchPackets: _pktMap List "
                      << "size is " << kv.second.size() << ".");
            std::unique_ptr<PacketJob<Sess>> pktJob(new PacketJob<Sess>());
            pktJob->bind(std::move(kv.second));
            (kv.first)->addJob(std::move(pktJob));
            kv.second.clear();
        }
    }
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

            // Append packet which needs to be sent to connections.
            handleJob();

            // Dispatch packet to back threads.
            dispatchPackets();

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
