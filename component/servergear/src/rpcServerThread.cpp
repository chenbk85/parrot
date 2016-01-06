#include "epoll.h"
#include "kqueue.h"
#include "config.h"
#include "rpcServerThread.h"
#include "logger.h"
#include "macroFuncs.h"
#include "timeoutManager.h"

namespace parrot
{
RpcServerThread::RpcServerThread(const Config* cfg)
    : ThreadBase(),
      TimeoutHandler<WsServerConn<RpcSession>>(),
      JobHandler(),
      ConnHandler<RpcServerConn>(),
      _connMap(),
      _registeredConnMap(),
      _reqMap(),
      _notifier(),
      _timeoutMgr(),
      _now(0),
      _random(),
      _rpcRspJobHdr(),
      _config(cfg)
{
    using namespace std::placeholders;
    _rpcRspJobHdr = std::bind(&RpcServerThread::handleRpcRsp, this, _1);

    init();
}

void RpcServerThread::init()
{
    _timeoutMgr.reset(new TimeoutManager<WsServerConn<RpcSession>>(
        this, _config->_rpcClientConnTimeout));

#if defined(__linux__)
    _notifier.reset(new Epoll(_config->_neighborSrvMap.size()));
#elif defined(__APPLE__)
    _notifier.reset(new Kqueue(_config->_neighborSrvMap.size()));
#elif defined(_WIN32)
//    _notifier.reset(new
//    SimpleEventNotifier(_config->_frontThreadMaxConnCount));
#endif
    _notifier->create();
}

void RpcServerThread::stop()
{
    ThreadBase::stop();
    _notifier->stopWaiting();
    ThreadBase::join();
    LOG_INFO("RpcServerThread::Stop: Done.");
}

void RpcServerThread::registerConn(const std::string& sid, RpcServerConn* conn)
{
    PARROT_ASSERT(_connMap.find(conn) != _connMap.end());

    auto it = _registeredConnMap.find(sid);
    if (it != _registeredConnMap.end())
    {
        // Found same server id!!! Kick this connection.
        LOG_WARN("RpcServerThread::registerConn: Removing conn "
                 << it->second->getSession()->toString() << ".");
        removeConn(it->second);
    }

    // Add the new connection to the connection map.
    _registeredConnMap[sid] = conn;
    LOG_DEBUG("RpcServerThread::addConn: Registered conn "
              << conn->getSession()->toString() << ".");
}

void RpcServerThread::addReqPacket(JobHandler* hdr,
                                   std::shared_ptr<RpcSession> rpcSession,
                                   std::unique_ptr<Json>&& cliSession,
                                   std::unique_ptr<WsPacket>&& pkt)
{
    _reqMap[hdr].emplace_back(std::move(rpcSession), std::move(cliSession),
                              std::move(pkt));
}

void RpcServerThread::handleRpcRsp(RspPktList& pktList)
{
    std::unordered_map<std::string, RpcServerConn*>::iterator it;
    for (auto& s : pktList)
    {
        it = _registeredConnMap.find((s.first)->getRemoteSid());
        if (it == _registeredConnMap.end())
        {
            LOG_WARN("RpcServerThread::handleRpcRsp: Failed to find remote sid "
                     << (s.first)->getRemoteSid() << ". Packet is discarded. "
                     << "SysJson is " << (s.second)->getSysJson()->toString()
                     << ".");
            continue;
        }

        it->second->sendPacket(s.second);
        if (it->second->canSwitchToSend())
        {
            it->second->setNextAction(eIoAction::Write);
            _notifier->updateEventAction(it->second);
        }

        LOG_DEBUG("RpcServerThread::handleRpcRsp: Sending packet. SysJson is "
                  << (s.second)->getSysJson()->toString() << ".");
    }
}

void RpcServerThread::dispatchPackets()
{
    for (auto& kv : _reqMap)
    {
        if (kv.second.empty())
        {
            continue;
        }

        std::unique_ptr<RpcSrvReqJob> job(new RpcSrvReqJob());
        job->bind(std::move(kv.second));
        kv.second.clear();
        (kv.first)->addJob(std::move(job));
    }
}

void RpcServerThread::addConnToNotifier()
{
    std::list<std::unique_ptr<RpcServerConn>> connList;
    _newConnListLock.lock();
    connList = std::move(_newConnList);
    _newConnListLock.unlock();

    for (auto& c : connList)
    {
        std::shared_ptr<RpcSession> sess(new RpcSession());
        c->setSession(std::move(sess));
        c->setNextAction(c->getDefaultAction());
        c->setRandom(&_random);

        _timeoutMgr->add(c.get(), _now);
        _notifier->addEvent(c.get());

        LOG_DEBUG("RpcServerThread::addConnToNotifier: Add client connection "
                  << c->getRemoteAddr() << ".");
        _connMap[c.get()] = std::move(c);
    }
}

void RpcServerThread::removeConn(RpcServerConn* conn)
{
    _connMap.erase(conn);
    _registeredConnMap.erase(conn->getSession()->getRemoteSid());
    _timeoutMgr->remove(conn);
    _notifier->delEvent(conn);
}

void RpcServerThread::updateTimeout(RpcServerConn* conn, std::time_t now)
{
    _timeoutMgr->update(conn, now);
}

void RpcServerThread::afterAddJob()
{
    _notifier->stopWaiting();
}

void RpcServerThread::afterAddNewConn()
{
    _notifier->stopWaiting();
}

void RpcServerThread::onTimeout(WsServerConn<RpcSession>* conn)
{
    LOG_WARN("RpcServerThread::onTimeout: Remote sid is "
             << conn->getSession()->toString() << ".");
    removeConn(static_cast<RpcServerConn*>(conn->getDerivedPtr()));
}

void RpcServerThread::handleJob()
{
    std::list<std::unique_ptr<Job>> jobList;
    _jobListLock.lock();
    jobList = std::move(_jobList);
    _jobListLock.unlock();

    for (auto& j : jobList)
    {
        switch (j->getJobType())
        {
            case JOB_RPC_SRV_RSP:
            {
                std::unique_ptr<RpcSrvRspJob> tj(static_cast<RpcSrvRspJob*>(
                    (j.release())->getDerivedPtr()));
                tj->call(_rpcRspJobHdr);
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

void RpcServerThread::run()
{
    uint32_t eventNum = 0;
    uint32_t idx      = 0;
    IoEvent* ev       = nullptr;
    eIoAction act     = eIoAction::None;

    try
    {
        while (!isStopping())
        {
            _now = std::time(nullptr);
            _timeoutMgr->checkTimeout(_now);

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
                            updateTimeout(static_cast<RpcServerConn*>(
                                              ev->getDerivedPtr()),
                                          _now);
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
                            static_cast<RpcServerConn*>(ev->getDerivedPtr()));
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
        LOG_ERROR("RpcServerThread::run: Errno is "
                  << e.code().message() << ". Meaning " << e.what());
        // There's nothing we can do here ...
        PARROT_ASSERT(false);
    }
}
}
