#include <system_error>

#include "eventNotifier.h"
#include "epoll.h"
#include "kqueue.h"
#include "simpleEventNotifier.h"

#include "config.h"
#include "wsServerConn.h"
#include "frontThread.h"
#include "macroFuncs.h"
#include "session.h"
#include "threadJob.h"

namespace parrot
{
FrontThread::FrontThread()
    : PoolThread(),
      _newConnListLock(),
      _newConnList(),
      _connMap(),
      _notifier(nullptr),
      _config(nullptr)
{
    using std::placeholders;
    _onPktHdr = std::bind(FrontThread::onPacket, this, _1, _2);
}

void FrontThread::setConfig(const Config* cfg)
{
    _config = cfg;
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

void FrontThread::addJob(std::unique_ptr<ThreadJob>&& job)
{
    _jobListLock.lock();
    _jobList.push_back(std::move(job));
    _jobListLock.unlock();
}

void FrontThread::addJob(std::list<std::unique_ptr<ThreadJob>>& jobList)
{
    _jobListLock.lock();
    _jobList.splice(_jobList.end(), jobList);
    _jobListLock.unlock();
}

void FrontThread::handleRspBind(std::shared_ptr<const Session>& ps)
{
    auto it = _connMap.find(ps->_connUniqueKey);

    if (it == _connMap.end())
    {
        LOG_WARN("FrontThread::handleRspBind: Failed to bind conn key"
                 << ps->_connUniqueKey << ". Session is "
                 << ps->toString() << ".");
        return;
    }

    it->second->getSession()->_isBound = true;
}

void FrontThread::handleUpdateSession(std::shared_ptr<const Session>& ps)
{
    auto it = _connMap.find(ps->_connUniqueKey);

    if (it == _connMap.end())
    {
        LOG_WARN("FrontThread::handleUpdateSession: Failed to bind conn key"
                 << ps->_connUniqueKey << ". Session is "
                 << ps->toString() << ".");
        return;
    }

    // Copy the the session. We need to save the _isBound first. Because
    // other thread may create the job when _isBound is not set.
    auto & session = it->second->getSession();
    bool isBound = session->_isBound;
    *session = *ps;
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
            case JobType::RspBind:
            {
                std::unqiue_ptr<RspBindJob> tj(
                    static_cast<RspBindJob*>(j->release()));
                tj->call(_rspBindHdr);
            }
            break;

            case JobType::UpdateSession:
            {
                std::unqiue_ptr<UpdateSessionJob> tj(
                    static_cast<UpdateSessionJob*>(j->release()));
                tj->call(_updateSessionHdr);
            }
            break;

            case JobType::Kick:
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

    if (it->second.size() >= Constants::kPktListSize)
    {
        // Dispatch packetes.
        (_pktHandlerFuncMap[treadPtr])(it->second);
        (_pktHandlerFuncMap[treadPtr]).clear();
    }
}

void FrontThread::dispatchPackets()
{
    std::unqiue_ptr<ReqBindJob> bindJob(new ReqBindJob(JobType::ReqBind));
    bindJob->bind(this, std::move(_noRoutePktList));
    _defaultPktHdr(std::move(bindJob));

    for (auto& kv : _threadPktMap)
    {
        if (!kv.second.empty())
        {
            (_pktHandlerFuncMap[kv.first])(kv.second);
            (_pktHandlerFuncMap[kv.first]).clear();
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

    for (auto& c : tmpList)
    {
        c->setAction(c->getDefaultAction());
        c->registerOnPacketCb(_onPktHdr);
        c->getSession()->_frontThreadPtr = this;
        c->setRandom(_random);

        _notifier->addEvent(c.get());
        _connMap[c->getUniqueKey()] = std::move(c);
    }
}

void FrontThread::run()
{
    uint32_t eventNum  = 0;
    uint32_t idx       = 0;
    WsServerConn* conn = nullptr;
    eIoAction act      = eIoAction::None;

    try
    {
        while (!isStopped())
        {
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
                    case eIoAction::Write:
                    case eIoAction::ReadWrite:
                    {
                        _notifier->updateEventAction(act);
                    }
                    break;

                    case eIoAction::Remove:
                    {
                        _notifier->delEvent(conn);
                        _connMap.erase(conn->getUniqueKey());
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
        LOG_ERROR("FrontThread::run: Errno is " << e.code() << ". Meaning "
                                                << e.what());
        // There's nothing we can do here ...
        PARROT_ASSERT(false);
    }
}
}
