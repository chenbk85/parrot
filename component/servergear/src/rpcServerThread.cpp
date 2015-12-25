#include "rpcServerThread.h"
#include "logger.h"

namespace parrot
{
RpcServerThread::RpcServerThread()
    : ThreadBase(),
      TimeoutHandler<RpcServerConn<RpcSession>>(),
      JobHandler(),
      ConnHandler<RpcServerConn<RpcSession>>(),
      _connMap(),
      _registeredConnMap(),
      _reqMap(),
      _notifier(),
      _timeoutMgr(),
      _rpcRspJobHdr()
{
    using namespace std::placeholders;
    _rpcRspJobHdr = std::bind(&RpcServerThread::handleRpcRsp, this, _1);
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
        LOG_WARN("RpcServerThread::registerConn: Removing conn "
                 << it->second->getSession()->toString() << ".");
        removeConn(it->second);
    }

    _registeredConnMap[sid] = conn;
    LOG_DEBUG("RpcServerThread::addConn: Registered conn "
              << conn->getSession()->toString() << ".");
}

void RpcServerThread::addReqPacket(JobHandler* hdr,
                                   std::shared_ptr<const RpcSession> rpcSession,
                                   std::unique_ptr<Json>&& cliSession,
                                   std::unique_ptr<WsPacket>&& pkt)
{
    _reqMap[hdr].emplace_back(std::move(rpcSession), std::move(cliSession),
                              std::move(pkt));
}

void RpcServerThread::handleRpcRsp(PktList& pktList)
{
    std::unordered_map<std::string, RpcSeverConn*>::iterator it;
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

        std::unique_ptr<RpcRequestJob> job(new RpcRequestJob());
        job.bind(std::move(kv.second));
        kv.second.clear();
        (kv.first)->addJob(std::move(job));
    }

    LOG_DEBUG("RpcServerThread::dispatchPackets");
}

void RpcServerThread::addConnToNotifier()
{
    std::list<std::unique_ptr<RpcServerConn>> connList;
    _newConnListLock.lock();
    connList = std::move(_newConnList);
    _newConnListLock.unlock();

    auto now = std::time(nullptr);
    for (auto& c : connList)
    {
        std::shared_ptr<RpcSession> sess(new RpcSession());
        c->setSession(std::move(sess));
        c->setNextAction(c->getDefaultAction());
        c->setRandom(&_random);

        _timeoutMgr->add(c.get(), now);
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
    _timeoutMgr.remove(conn);
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

void RpcServerThread::onTimeout(RpcServerConn* conn)
{
    LOG_WARN("RpcServerThread::onTimeout: Remote sid is "
             << conn->getSession()->getRemoteSid() << ".");
    std::unique_ptr<WsPacket> pkt(new WsPacket());
    pkt->setOpCode(eOpCode::Close);
    pkt->setClose(eCodes::ERR_Timeout);
    conn->sendPacket(pkt);
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
            case JOB_RPC_RSP:
            {
                std::unique_ptr<RpcResponseJob> tj(static_cast<RpcResponseJob*>(
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
                                static_cast<RpcSeverConn*>(ev->getDerivedPtr()),
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
