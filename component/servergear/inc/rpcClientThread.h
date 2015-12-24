#ifndef __COMPONENT_SERVERGEAR_INC_RPCCLIENTTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_RPCCLIENTTHREAD_H__

#include <memory>
#include <list>

#include "threadBase.h"
#include "mtRandom.h"
#include "jobHandler.h"
#include "threadJob.h"
#include "timeoutHandler.h"
#include "timeoutManager.h"
#include "rpcClientConn.h"
#include "eventNotifier.h"
#include "wsPacket.h"
#include "connHandler.h"
#include "epoll.h"
#include "kqueue.h"

namespace parrot
{

template <typename Sess>
class RpcClientThread : public ThreadBase,
                        public TimeoutHandler<RpcClientConn<Sess>>,
                        public JobHandler
{
    using ConnMap =
        std::unordered_map<std::string, std::shared_ptr<RpcClientConn<Sess>>>;

  public:
    RpcClientThread(const Config& cfg, const WsConfig& wsCfg);

  public:
    void addJob(std::unique_ptr<Job>&& job);
    void addJob(std::list<std::unique_ptr<Job>>& jobList);
    void addRsp(JobHandler* hdr,
                std::shared_ptr<Sess>&,
                std::std::unique_ptr<WsPacket>&& pkt);

  private:
    void init();
    void doConnect();
    void handleRsp();

  private:
    void onTimeout() override;
    void beforeStart() override;
    void run() override;

  private:
    std::mutex _jobListLock;
    std::unordered_map<std::string, std::unique_ptr<job>> _jobList;

    std::list<std::unique_ptr<RpcClientConn<Sess>>> _disconnectedConnList;

    // Save connected client to this map.
    ConnMap _connMap;

    std::unordered_map<JobHandler*, std::list<SessionPktPair>> _rspMap;

    std::unique_ptr<EventNotifier> _notifier;
    std::unique_ptr<TimeoutManager<RpcClientConn<Sess>>> _timeoutMgr;
    MtRandom _random;
    const Config& _config;
    const WsConfig& _wsConfig;
};

template <typename Sess>
RpcClientThread<Sess>::RpcClientThread(const Config& cfg, const WsConfig& wsCfg)
    : ThreadBase(),
      TimeoutHandler<RpcClientConn<Sess>>(),
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

template <typename Sess> void RpcClientConn<Sess>::init()
{
    _timeoutMgr.reset(new TimeoutManager<RpcClientConn<Sess>>(
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

template <typename Sess>
void RpcClientThread<Sess>::addJob(std::unique_ptr<Job>&& job)
{
    _jobListLock.lock();
    _jobList.push_back(std::move(job));
    _jobListLock.unlock();

    _notifier.stopWaiting();
}

template <typename Sess>
void RpcClientThread<Sess>::addJob(std::list<std::unique_ptr<Job>>& jobList)
{
    _jobListLock.lock();
    _jobList.splice(_jobList.end(), jobList);
    _jobListLock.unlock();

    _notifier.stopWaiting();
}

template <typename Sess>
void RpcClientThread<Sess>::addRsp(JobHandler* hdr,
                                   std::shared_ptr<Sess>& session,
                                   std::std::unique_ptr<WsPacket>&& pkt)
{
    _rpcMap[hdr].emplace_back(std::move(session), std::move(pkt));
}

template <typename Sess> void RpcClientThread<Sess>::doConnect()
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

template <typename Sess> void RpcClientThread<Sess>::checkRpcRequestTimeout()
{
    for (auto& c : _connMap)
    {
        (c.second)->checkReqTimeout();
    }
}

template <typename Sess> void RpcClientThread<Sess>::handleRsp()
{
    for (auto& kv : rspMap)
    {
        if (kv.second.empty())
        {
            continue;
        }

        std::unique_ptr<PacketJob> pktJob(new PacketJob());
        pktJob.bind(std::move(kv.second));
        kv.second.clear();
        (kv.first)->addJob(std::move(pktJob));
    }
}

template <typename Sess>
void RpcClientThread<Sess>::updateTimeout(RpcClientConn<Sess>* conn,
                                          std::time_t now)
{
    _timeoutMgr->update(conn, now);
}

template <typename Sess>
void RpcClientThread<Sess>::onTimeout(RpcClientConn<Sess>* conn)
{
    conn->heartbeat();
}

template <typename Sess> void RpcClientThread<Sess>::beforeStart()
{
    std::time_t now = std::time(nullptr);

    for (const auto& s : _config._neighborSrvMap)
    {
        std::unique_ptr<RpcClientConn<Sess>> conn(new RpcClientConn<Sess>(
            this, _config, s._thisServer.rpcWsUrl, _wsConfig));

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

template <typename Sess> void RpcClientThread<Sess>::run()
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
        LOG_ERROR("RpcClientThread::run: Errno is "
                  << e.code().message() << ". Meaning " << e.what());
        // There's nothing we can do here ...
        PARROT_ASSERT(false);
    }
}
}

#endif
