#include <system_error>
#include <string>
#include <ctime>
#include <iostream>

#include "wsPacket.h"
#include "eventNotifier.h"
#include "epoll.h"
#include "kqueue.h"

#include "mtRandom.h"
#include "config.h"
#include "wsServerConn.h"
#include "frontThread.h"
#include "macroFuncs.h"
#include "session.h"
#include "threadJob.h"
#include "timeoutManager.h"
#include "logger.h"

namespace parrot
{
FrontThread::FrontThread()
    : PoolThread(),
      TimeoutHandler(),
      _jobListLock(),
      _jobList(),
      _connListLock(),
      _connList(),
      _noRoutePktList(),
      _pktMap(),
      _jobHandlerSet(),
      _connMap(),
      _notifier(nullptr),
      _defaultJobHdrVec(),
      _lastDefaultJobIdx(0),
      _rspBindHdr(),
      _updateSessionHdr(),
      _pktJobHdr(),
      _random(),
      _timeoutMgr(),
      _config(nullptr)
{
    using namespace std::placeholders;

    _rspBindHdr       = std::bind(&FrontThread::handleRspBind, this, _1);
    _updateSessionHdr = std::bind(&FrontThread::handleUpdateSession, this, _1);
    _pktJobHdr        = std::bind(&FrontThread::handlePacket, this, _1);
}

void FrontThread::updateByConfig(const Config* cfg)
{
    _config = cfg;
    _timeoutMgr.reset(
        new TimeoutManager<WsServerConn>(this, _config->_frontThreadTimeout));

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

void FrontThread::setDefaultJobHdr(std::vector<JobHandler*>& vec)
{
    _defaultJobHdrVec = vec;
}

void FrontThread::setJobHdr(std::unordered_set<JobHandler*>& hdr)
{
    LOG_DEBUG("FrontThread::setJobHdr: Handler set size is " << hdr.size()
                                                             << ".");
    _jobHandlerSet = hdr;
}

void FrontThread::stop()
{
    ThreadBase::stop();
    _notifier->stopWaiting();
    ThreadBase::join();
    LOG_INFO("FrontThread::Stop: Done.");
}

void FrontThread::handleRspBind(std::list<std::shared_ptr<const Session>>& sl)
{
    std::unordered_map<uint64_t, std::unique_ptr<WsServerConn>>::iterator it;
    for (auto& s : sl)
    {
        it = _connMap.find(s->_connUniqueId);
        if (it == _connMap.end())
        {
            LOG_WARN("FrontThread::handleRspBind: Failed to bind conn key"
                     << s->_connUniqueId << ". Session is " << s->toString()
                     << ".");
            continue;
        }
        it->second->getSession()->_isBound = true;
    }
}

void FrontThread::handleUpdateSession(std::shared_ptr<const Session>& ps)
{
    auto it = _connMap.find(ps->_connUniqueId);

    if (it == _connMap.end())
    {
        LOG_WARN("FrontThread::handleUpdateSession: Failed to bind conn key"
                 << ps->_connUniqueId << ". Session is " << ps->toString()
                 << ".");
        return;
    }

    // Copy the the session. We need to save the _isBound first. Because
    // other thread may create the job when _isBound is not set.
    auto& session     = it->second->getSession();
    bool isBound      = session->_isBound;
    *session          = *ps;
    session->_isBound = isBound;
}

void FrontThread::handlePacket(std::list<SessionPktPair>& pktList)
{
    std::unordered_map<uint64_t, std::unique_ptr<WsServerConn>>::iterator it;
    for (auto& s : pktList)
    {
        it = _connMap.find((s.first)->_connUniqueId);
        if (it == _connMap.end())
        {
            LOG_DEBUG("FrontThread::handlePacket: Failed to find unique "
                      << "connection id: " << (s.first)->_connUniqueId << ".");
            continue;
        }

        LOG_DEBUG("FrontThread::handlePacket: Send packet to conn "
                  << (s.first)->_connUniqueId);
        it->second->sendPacket(s.second);

        if (it->second->canSwitchToSend())
        {
            it->second->setNextAction(eIoAction::Write);
            _notifier->updateEventAction(it->second.get());
        }
    }
}

void FrontThread::handleJob()
{
    std::list<std::unique_ptr<Job>> jobList;
    _jobListLock.lock();
    jobList = std::move(_jobList);
    _jobListLock.unlock();

    for (auto& j : jobList)
    {
        switch (j->getJobType())
        {
            case eJobType::RspBind:
            {
                std::unique_ptr<RspBindJob> tj(
                    static_cast<RspBindJob*>(j.release()));
                tj->call(_rspBindHdr);
            }
            break;

            case eJobType::UpdateSession:
            {
                std::unique_ptr<UpdateSessionJob> tj(
                    static_cast<UpdateSessionJob*>(j.release()));
                tj->call(_updateSessionHdr);
            }
            break;

            case eJobType::Packet:
            {
                std::unique_ptr<PacketJob> tj(
                    static_cast<PacketJob*>(j.release()));
                tj->call(_pktJobHdr);
            }
            break;

            case eJobType::Kick:
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

void FrontThread::addJob(std::unique_ptr<Job>&& job)
{
    LOG_DEBUG("FrontThread::addJob.");
    _jobListLock.lock();
    _jobList.push_back(std::move(job));
    _jobListLock.unlock();
    _notifier->stopWaiting();
}

void FrontThread::addJob(std::list<std::unique_ptr<Job>>& jobList)
{
    LOG_DEBUG("FrontThread::addJob: JobList size is " << jobList.size() << ".");
    _jobListLock.lock();
    _jobList.splice(_jobList.end(), jobList);
    _jobListLock.unlock();
    _notifier->stopWaiting();
}

void FrontThread::onPacket(std::shared_ptr<const Session>&& session,
                           std::unique_ptr<WsPacket>&& pkt)
{
    if (!session->_isBound)
    {
        _noRoutePktList.emplace_back(std::move(session), std::move(pkt));
        return;
    }

    auto it = _pktMap.find(session->_jobHandlerPtr);

    if (it == _pktMap.end())
    {
        PARROT_ASSERT(session->_jobHandlerPtr);
        LOG_DEBUG("FrontThread::onPacket: First add thread packet.");
        _pktMap[session->_jobHandlerPtr].emplace_back(std::move(session),
                                                      std::move(pkt));
    }
    else
    {
        LOG_DEBUG("FrontThread::onPacket: Append packet.");
        it->second.emplace_back(std::move(session), std::move(pkt));
        if (it->second.size() >= static_cast<uint32_t>(Constants::kPktListSize))
        {
            // Dispatch packetes.
            std::unique_ptr<PacketJob> pktJob(new PacketJob());
            pktJob->bind(std::move(it->second));
            auto sit = _jobHandlerSet.find(
                static_cast<JobHandler*>(session->_jobHandlerPtr));
            (*sit)->addJob(std::move(pktJob));
            it->second.clear();
        }
    }
}

void FrontThread::onClose(WsServerConn* conn, std::unique_ptr<WsPacket>&& pkt)
{
    std::shared_ptr<const Session> session = conn->getSession();

    if (!session->_isBound)
    {
        // Here, do not need notify up layer.
        removeConn(conn);
        LOG_INFO("FrontThread::onClose: Not bind. Err is "
                 << (uint32_t)pkt->getCloseCode() << ". Session is "
                 << session->toString());
        return;
    }

    auto it = _pktMap.find(session->_jobHandlerPtr);

    if (it == _pktMap.end())
    {
        _pktMap[session->_jobHandlerPtr].emplace_back(std::move(session),
                                                      std::move(pkt));
    }
    else
    {
        it->second.emplace_back(std::move(session), std::move(pkt));
        if (it->second.size() >= static_cast<uint32_t>(Constants::kPktListSize))
        {
            // Dispatch packetes.
            std::unique_ptr<PacketJob> pktJob(new PacketJob());
            pktJob->bind(std::move(it->second));
            auto sit = _jobHandlerSet.find(
                static_cast<JobHandler*>(session->_jobHandlerPtr));
            (*sit)->addJob(std::move(pktJob));
            it->second.clear();
        }
    }

    removeConn(conn);
    LOG_INFO("FrontThread::onClose: Bound. Err is "
             << (uint32_t)(pkt->getCloseCode()) << ". Session is "
             << session->toString());
}

void FrontThread::dispatchPackets()
{
    if (!_noRoutePktList.empty())
    {
        LOG_DEBUG("FrontThread::dispatchPackets: _noRoutePktList size is "
                  << _noRoutePktList.size() << ".");
        std::unique_ptr<ReqBindJob> bindJob(new ReqBindJob());
        bindJob->bind(this, std::move(_noRoutePktList));
        (_defaultJobHdrVec[_lastDefaultJobIdx])->addJob(std::move(bindJob));
        _lastDefaultJobIdx =
            (_lastDefaultJobIdx + 1) % _defaultJobHdrVec.size();
    }

    for (auto& kv : _pktMap)
    {
        if (!kv.second.empty())
        {
            LOG_DEBUG("FrontThread::dispatchPackets: _pktMap List "
                      << "size is " << kv.second.size() << ".");
            std::unique_ptr<PacketJob> pktJob(new PacketJob());
            pktJob->bind(std::move(kv.second));
            auto sit = _jobHandlerSet.find(static_cast<JobHandler*>(kv.first));
            (*sit)->addJob(std::move(pktJob));
            kv.second.clear();
        }
    }
}

void FrontThread::addConn(std::list<std::unique_ptr<WsServerConn>>& connList)
{
    for (auto& c : connList)
    {
        LOG_DEBUG("FrontThread::addConn: connId is "
                  << c->getSession()->_connUniqueId);
    }
    _connListLock.lock();
    _connList.splice(_connList.end(), connList);
    _connListLock.unlock();
    _notifier->stopWaiting();
}

void FrontThread::addConnToNotifier()
{
    std::list<std::unique_ptr<WsServerConn>> tmpList;

    _connListLock.lock();
    tmpList = std::move(_connList);
    _connListLock.unlock();

    auto now = std::time(nullptr);

    for (auto& c : tmpList)
    {
        c->setNextAction(c->getDefaultAction());
        c->setPacketHandler(this);
        c->setRandom(&_random);

        // Set front thread pointer in session.
        c->getSession()->_frontThreadPtr = this;

        _timeoutMgr->add(c.get(), now);
        _notifier->addEvent(c.get());

        LOG_DEBUG("FrontThread::addConnToNotifier: Add client connection "
                  << c->getRemoteAddr() << ".");
        _connMap[c->getSession()->_connUniqueId] = std::move(c);
    }
}

void FrontThread::removeConn(WsServerConn* conn)
{
    LOG_DEBUG("FrontThread::removeConn: Client " << conn->getRemoteAddr()
                                                 << " disconnected.");
    _timeoutMgr->remove(conn);
    _notifier->delEvent(conn);
    _connMap.erase(conn->getSession()->_connUniqueId);
}

void FrontThread::updateTimeout(WsServerConn* conn, std::time_t now)
{
    _timeoutMgr->update(conn, now);
}

void FrontThread::onTimeout(WsServerConn* conn)
{
    std::unique_ptr<WsPacket> pkt(new WsPacket());
    pkt->setOpCode(eOpCode::Close);
    pkt->setClose(eCodes::ERR_Timeout);
    onClose(conn, std::move(pkt));
}

void FrontThread::run()
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

            // Append packet which needs to be sent to connections.
            handleJob();

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
                            updateTimeout(
                                static_cast<WsServerConn*>(ev->getDerivedPtr()),
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
                        removeConn(
                            static_cast<WsServerConn*>(ev->getDerivedPtr()));
                    }
                    break;

                    default:
                    {
                        PARROT_ASSERT(false);
                    }
                    break;
                } // switch
            }     // for

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
