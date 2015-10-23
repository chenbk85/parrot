#include <system_error>
#include <string>
#include <ctime>

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
      _noRoutePktList(),
      _pktMap(),
      _jobHandlerMap(),
      _threadJobMap(),
      _newConnListLock(),
      _newConnList(),
      _connMap(),
      _notifier(nullptr),
      _defaultJobHdrVec(),
      _lastDefaultJobIdx(0),
      _rspBindHdr(),
      _updateSessionHdr(),
      _random(),
      _timeoutMgr(),
      _config(nullptr)
{
    using namespace std::placeholders;

    _rspBindHdr       = std::bind(&FrontThread::handleRspBind, this, _1);
    _updateSessionHdr = std::bind(&FrontThread::handleUpdateSession, this, _1);
}

void FrontThread::updateByConfig(const Config* cfg)
{
    _config = cfg;
    _timeoutMgr.reset(
        new TimeoutManager<WsServerConn>(this, _config->_frontThreadTimeout));
}

void FrontThread::setDefaultJobHdr(std::vector<JobHandler*>& vec)
{
    _defaultJobHdrVec = vec;
}

void FrontThread::beforeStart()
{
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

void FrontThread::stop()
{
    _notifier->stopWaiting();
    ThreadBase::stop();
}

void FrontThread::handleRspBind(std::list<std::shared_ptr<const Session>>& sl)
{
    std::unordered_map<uint64_t, std::shared_ptr<WsServerConn>>::iterator it;
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

void FrontThread::onPacket(std::shared_ptr<const Session>&& session,
                           std::unique_ptr<WsPacket>&& pkt)
{
    if (!session->_isBound)
    {
        _noRoutePktList.emplace_back(std::move(session), std::move(pkt));
        return;
    }

    auto it = _pktMap.find(session->_backThreadPtr);

    if (it == _pktMap.end())
    {
        _pktMap[session->_backThreadPtr].emplace_back(std::move(session),
                                                      std::move(pkt));
    }
    else
    {
        it->second.emplace_back(std::move(session), std::move(pkt));
    }

    if (it->second.size() >= static_cast<uint32_t>(Constants::kPktListSize))
    {
        // Dispatch packetes.
        std::unique_ptr<PacketJob> pktJob(new PacketJob(eJobType::Packet));
        pktJob->bind(std::move(it->second));
        (_jobHandlerMap[session->_backThreadPtr])->addJob(std::move(pktJob));
        it->second.clear();
    }
}

void FrontThread::onClose(WsServerConn* conn, std::unique_ptr<WsPacket> &&pkt)
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

    auto it = _pktMap.find(session->_backThreadPtr);

    if (it == _pktMap.end())
    {
        _pktMap[session->_backThreadPtr].emplace_back(std::move(session),
                                                      std::move(pkt));
    }
    else
    {
        it->second.emplace_back(std::move(session), std::move(pkt));
    }

    if (it->second.size() >= static_cast<uint32_t>(Constants::kPktListSize))
    {
        // Dispatch packetes.
        std::unique_ptr<PacketJob> pktJob(new PacketJob(eJobType::Packet));
        pktJob->bind(std::move(it->second));
        (_jobHandlerMap[session->_backThreadPtr])->addJob(std::move(pktJob));
        it->second.clear();
    }

    removeConn(conn);
    LOG_INFO("FrontThread::onClose: Bound. Err is "
             << (uint32_t)pkt->getCloseCode() << ". Session is "
             << session->toString());
}

void FrontThread::dispatchPackets()
{
    std::unique_ptr<ReqBindJob> bindJob(new ReqBindJob(eJobType::ReqBind));
    bindJob->bind(this, std::move(_noRoutePktList));
    (_defaultJobHdrVec[_lastDefaultJobIdx])->addJob(std::move(bindJob));
    _lastDefaultJobIdx = (_lastDefaultJobIdx + 1) % _defaultJobHdrVec.size();

    for (auto& kv : _threadJobMap)
    {
        if (!kv.second.empty())
        {
            std::unique_ptr<PacketJob> pktJob(
                new PacketJob(eJobType::Packet));
            pktJob->bind(std::move(kv.second));
            (_jobHandlerMap[kv.first])->addJob(std::move(pktJob));
            kv.second.clear();
        }
    }
}

void FrontThread::addConn(std::list<std::shared_ptr<WsServerConn>>& connList)
{
    _newConnListLock.lock();
    // Append the connList to the _newConnList.
    _newConnList.splice(_newConnList.end(), connList);
    _newConnListLock.unlock();

    _notifier->stopWaiting();
}

void FrontThread::addConnToNotifier()
{
    std::list<std::shared_ptr<WsServerConn>> tmpList;

    _newConnListLock.lock();
    std::swap(tmpList, _newConnList);
    _newConnListLock.unlock();

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
        _connMap[c->getUniqueKey()] = std::move(c);
    }
}

void FrontThread::removeConn(WsServerConn* conn)
{
    _connMap.erase(conn->getSession()->_connUniqueId);
    _timeoutMgr->remove(conn);
    _notifier->delEvent(conn);
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
    uint32_t eventNum  = 0;
    uint32_t idx       = 0;
    WsServerConn* conn = nullptr;
    eIoAction act      = eIoAction::None;
    std::time_t now    = 0;

    try
    {
        while (!isStopped())
        {
            now = std::time(nullptr);
            _timeoutMgr->checkTimeout(now);

            addConnToNotifier();

            eventNum = _notifier->waitIoEvents(-1);

            for (idx = 0; idx != eventNum; ++idx)
            {
                // We are sure that the IoEvnet is WsServerConn,
                // so we can use static_cast.
                conn = static_cast<WsServerConn*>(_notifier->getIoEvent(idx));
                act = conn->handleIoEvent();
                conn->setNextAction(act);

                switch (act)
                {
                    case eIoAction::Read:
                    case eIoAction::ReadWrite:
                    {
                        updateTimeout(conn, now);
                    }
                    // No break;
                    case eIoAction::Write:
                    {
                        _notifier->updateEventAction(conn);
                    }
                    break;

                    case eIoAction::Remove:
                    {
                        removeConn(conn);
                    }
                    break;

                    default:
                    {
                        PARROT_ASSERT(false);
                    }
                    break;
                } // switch
            }     // for

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
