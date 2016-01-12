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
      JobManager(),
      ConnHandler<RpcServerConn>(),
      _connMap(),
      _registeredConnMap(),
      _jobProcesser(),
      _notifier(),
      _timeoutMgr(),
      _now(0),
      _random(),
      _config(cfg)
{
    init();
}

void RpcServerThread::init()
{
    _jobProcesser.reset(new RpcServerJobProcesser(this));

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
    setJobProcesser(_jobProcesser.get());    
    JobManager::setEventNotifier(_notifier.get());
    ConnHandler::setEventNotifier(_notifier.get());    
}

void RpcServerThread::stop()
{
    ThreadBase::stop();
    _notifier->stopWaiting();
    ThreadBase::join();
    LOG_INFO("RpcServerThread::stop: Done.");
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

void RpcServerThread::addReqPacket(JobManager* mgr,
                                   RpcSrvReqJobParam&& jobParam)
{
    _jobProcesser->createRpcReqJob(mgr, std::move(jobParam));
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

void RpcServerThread::onTimeout(WsServerConn<RpcSession>* conn)
{
    LOG_WARN("RpcServerThread::onTimeout: Remote sid is "
             << conn->getSession()->toString() << ".");
    removeConn(static_cast<RpcServerConn*>(conn->getDerivedPtr()));
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
            handleJobs();
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
