#include "rpcClientThread.h"
#include "epoll.h"
#include "kqueue.h"

namespace parrot
{
RpcClientThread::RpcClientThread(const Config& cfg, const WsConfig& wsCfg)
    : ThreadBase(),
      TimeoutHandler<RpcClientConn>(),
      JobHandler(),
      _jobListLock(),
      _jobList(),
      _disconnectedConnList(),
      _connMap(),
      _notifier(),
      _timeoutMgr(),
      _random(),
      _config(cfg),
      _wsConfig(wsCfg)
{
    init();
}

void RpcClientConn::init()
{
    _timeoutMgr.reset(new TimeoutManager<RpcClientConn<RpcSession>>(
        this, _config._rpcThreadTimeout));

#if defined(__linux__)
    _notifier.reset(new Epoll(_config._neighborSrvMap.count()));
#elif defined(__APPLE__)
    _notifier.reset(new Kqueue(_config._neighborSrvMap.count()));
#elif defined(_WIN32)
//    _notifier.reset(new
//    SimpleEventNotifier(_config->_frontThreadMaxConnCount));
#endif
    _notifier->create();
}

void RpcClientThread::addJob(std::unique_ptr<Job>&& job)
{
    _jobListLock.lock();
    _jobList.push_back(std::move(job));
    _jobListLock.unlock();

    _notifier.stopWaiting();
}

void RpcClientThread::addJob(std::list<std::unique_ptr<Job>>& jobList)
{
    _jobListLock.lock();
    _jobList.splice(_jobList.end(), jobList);
    _jobListLock.unlock();

    _notifier.stopWaiting();
}

void RpcClientThread::doConnect()
{
    if (_disconnectedConnList.empty())
    {
        return;
    }

    std::time_t now = std::time(nullptr);
    eIoAction act;

    for (auto it = _disconnectedConnList.begin();
         it != _disconnectedConnList.end();)
    {
        if (!it->canConnect())
        {
            ++it;
            continue;
        }

        act = (*it)->handleIoEvent(); // Initial connecting.
        (*it)->setNextAction(act);
        _timeoutMgr->add((*it).get(), now);
        _notifier->addEvent((*it).get());
        _disconnectedConnList.remove(it);
    }
}

void RpcClientThread::checkRpcRequestTimeout()
{
    for (auto& c : _connMap)
    {
        (c.second)->checkReqTimeout();
    }
}

void RpcClientThread::updateTimeout(RpcClientConn<RpcSession>* conn,
                                    std::time_t now)
{
    _timeoutMgr->update(conn, now);
}

void RpcClientThread::onTimeout(RpcClientConn<RpcSession>* conn)
{
    conn->heartbeat();
}

void RpcClientThread::beforeStart()
{
    std::time_t now = std::time(nullptr);
    
    for (const auto& s : _config._neighborSrvMap)
    {
        std::unique_ptr<RpcClientConn> conn(
            new RpcClientConn(_config, s._thisServer.rpcWsUrl, _wsConfig));

        conn->setDerivedPtr(conn.get());
        conn->setNextAction(conn->getDefaultAction());
        conn->setPacketHandler(conn);
        conn->setRandom(&_random);
        conn->handleIoEvent(); // Initial connecting.
        _timeoutMgr->add(conn.get(), now);
        _notifier->addEvent(conn.get());
        _disconnectedConnList.push_back(std::move(conn));
    }
}

void RpcClientThread::run()
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
            doConnect();

            now = std::time(nullptr);
            _timeoutMgr->checkTimeout(now);

            // Needs to wake up to reconnect. So we should use small
            // milliseconds.
            eventNum = _notifier->waitIoEvents(1000); 

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
                            updateTimeout(static_cast<WsServerConn<Session>*>(
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
                        removeConn(static_cast<WsServerConn<Session>*>(
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

            checkRpcRequestTimeout();

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
