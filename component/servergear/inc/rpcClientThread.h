#ifndef __COMPONENT_SERVERGEAR_INC_RPCCLIENTTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_RPCCLIENTTHREAD_H__

#include <memory>
#include <list>

#include "codes.h"
#include "threadBase.h"
#include "mtRandom.h"
#include "jobHandler.h"
#include "threadJob.h"
#include "timeoutHandler.h"
#include "timeoutManager.h"
#include "eventNotifier.h"
#include "wsPacket.h"
#include "connHandler.h"
#include "epoll.h"
#include "kqueue.h"
#include "rpcSession.h"
#include "wsClientConn.h"
#include "config.h"
#include "wsConfig.h"
#include "rpcRequest.h"
#include "jobFactory.h"

namespace parrot
{

template <typename Sess> class RpcClientConn;

template <typename Sess>
class RpcClientThread : public ThreadBase,
                        public TimeoutHandler<WsClientConn<RpcSession>>,
                        public JobHandler
{
    using ConnMap =
        std::unordered_map<std::string, std::shared_ptr<RpcClientConn<Sess>>>;

    using RpcCliRspJobParam = std::tuple<eCodes,
                                         std::shared_ptr<const Sess>,
                                         std::unique_ptr<WsPacket>>;
    using RpcCliRspJobFactory =
        JobFactory<RpcCliRspJobParam, RpcCliRspJob<Sess>>;
    using HdrJobListMap =
        std::unordered_map<JobHandler*, std::list<std::unique_ptr<Job>>>;

  public:
    RpcClientThread(const Config& cfg, const WsConfig& wsCfg);

  public:
    // Internal API. Client should not call this API.
    void addRsp(JobHandler* hdr,
                eCodes code,
                std::shared_ptr<const Sess>&,
                std::unique_ptr<WsPacket>&& pkt);

  public:
    void stop() override;

  private:
    void init();
    void doConnect();
    void handleRsp();
    void checkRpcRequestTimeout();
    void updateTimeout(WsClientConn<RpcSession>* conn, std::time_t now);

  private:
    void afterAddJob() override;
    void handleJob() override;
    void onTimeout(WsClientConn<RpcSession>*) override;
    void beforeStart() override;
    void run() override;

  private:
    std::list<std::unique_ptr<RpcClientConn<Sess>>> _disconnectedConnList;

    // Save connected client to this map.
    ConnMap _connMap;

    RpcCliRspJobFactory _rpcCliRspJobFactory;
    HdrJobListMap _hdrJobListMap;

    std::unique_ptr<EventNotifier> _notifier;
    std::unique_ptr<TimeoutManager<WsClientConn<RpcSession>>> _timeoutMgr;
    MtRandom _random;
    std::time_t _now;
    const Config& _config;
    const WsConfig& _wsConfig;
};

template <typename Sess>
RpcClientThread<Sess>::RpcClientThread(const Config& cfg, const WsConfig& wsCfg)
    : ThreadBase(),
      TimeoutHandler<WsClientConn<RpcSession>>(),
      JobHandler(),
      _disconnectedConnList(),
      _connMap(),
      _rpcCliRspJobFactory(),
      _hdrJobListMap(),
      _notifier(),
      _timeoutMgr(),
      _random(),
      _now(0),
      _config(cfg),
      _wsConfig(wsCfg)
{
    init();
}

template <typename Sess> void RpcClientThread<Sess>::init()
{
    _timeoutMgr.reset(new TimeoutManager<WsClientConn<RpcSession>>(
        this, _config._rpcClientHeartbeatInterval));

#if defined(__linux__)
    _notifier.reset(new Epoll(_config._neighborSrvMap.size()));
#elif defined(__APPLE__)
    _notifier.reset(new Kqueue(_config._neighborSrvMap.size()));
#elif defined(_WIN32)
//    _notifier.reset(new
//    SimpleEventNotifier(_config->_frontThreadMaxConnCount));
#endif
    _notifier->create();
}

template <typename Sess> void RpcClientThread<Sess>::stop()
{
    ThreadBase::stop();
    _notifier->stopWaiting();
    ThreadBase::join();
    LOG_INFO("RpcClientThread::stop: Done.");
}

template <typename Sess> void RpcClientThread<Sess>::afterAddJob()
{
    _notifier->stopWaiting();
}

template <typename Sess>
void RpcClientThread<Sess>::addRsp(JobHandler* hdr,
                                   eCodes code,
                                   std::shared_ptr<const Sess>& session,
                                   std::unique_ptr<WsPacket>&& pkt)
{
    _rpcCliRspJobFactory.add(
        hdr, RpcCliRspJobParam(code, std::move(session), std::move(pkt)));
}

template <typename Sess> void RpcClientThread<Sess>::doConnect()
{
    if (_disconnectedConnList.empty())
    {
        return;
    }

    eIoAction act;

    for (auto it = _disconnectedConnList.begin();
         it != _disconnectedConnList.end();)
    {
        if (!(*it)->canConnect())
        {
            ++it;
            continue;
        }

        act = (*it)->handleIoEvent(); // Initial connecting.
        (*it)->setNextAction(act);
        _timeoutMgr->add((*it).get(), _now);
        _notifier->addEvent((*it).get());
        _disconnectedConnList.erase(it);
    }
}

template <typename Sess> void RpcClientThread<Sess>::checkRpcRequestTimeout()
{
    for (auto& c : _connMap)
    {
        (c.second)->checkReqTimeout(_now);
    }
}

template <typename Sess> void RpcClientThread<Sess>::handleRsp()
{
    _rpcCliRspJobFactory.loadJobs(_hdrJobListMap);

    for (auto& kv : _hdrJobListMap)
    {
        if (kv.second.empty())
        {
            continue;
        }
        (kv.first)->addJob(kv.second);
        kv.second.clear();
    }
}

template <typename Sess>
void RpcClientThread<Sess>::updateTimeout(WsClientConn<RpcSession>* conn,
                                          std::time_t now)
{
    _timeoutMgr->update(conn, now);
}

template <typename Sess>
void RpcClientThread<Sess>::onTimeout(WsClientConn<RpcSession>* conn)
{
    (static_cast<RpcClientConn<Sess>*>(conn->getDerivedPtr()))->heartbeat();
}

template <typename Sess> void RpcClientThread<Sess>::beforeStart()
{
    std::time_t now = std::time(nullptr);

    for (const auto& s : _config._neighborSrvMap)
    {
        std::unique_ptr<RpcClientConn<Sess>> conn(new RpcClientConn<Sess>(
            this, _config, s.second._rpcWsUrl, _wsConfig));

        conn->setDerivedPtr(conn.get());
        conn->setNextAction(conn->getDefaultAction());
        conn->setRandom(&_random);
        conn->handleIoEvent(); // Initial connecting.
        _timeoutMgr->add(conn.get(), now);
        _notifier->addEvent(conn.get());
        _disconnectedConnList.push_back(std::move(conn));
    }
}

template <typename Sess> void RpcClientThread<Sess>::handleJob()
{
    std::list<std::unique_ptr<Job>> jobList;
    _jobListLock.lock();
    jobList = std::move(_jobList);
    _jobListLock.unlock();

    auto mit = _connMap.end();

    for (auto& j : jobList)
    {
        switch (j->getJobType())
        {
            case JOB_RPC_CLI_REQ:
            {
                std::unique_ptr<RpcRequest<Sess>> req(
                    static_cast<RpcRequest<Sess>*>(
                        (j.release())->getDerivedPtr()));

                mit = _connMap.find(req->getDestSrvId());
                if (mit == _connMap.end())
                {
                    LOG_WARN(
                        "RpcClientThread::handleJob: Failed to find rpc server "
                        << req->getDestSrvId() << ".");
                    addRsp(req->getRspHandler(), eCodes::ERR_RemoteNotConnected,
                           req->getSession(),
                           std::unique_ptr<WsPacket>(new WsPacket()));
                    continue;
                }

                mit->second->addJob(req);
                if (mit->second->canSwitchToSend())
                {
                    mit->second->setNextAction(eIoAction::Write);
                    _notifier->updateEventAction(mit->second.get());
                }
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

template <typename Sess> void RpcClientThread<Sess>::run()
{
    uint32_t eventNum = 0;
    uint32_t idx      = 0;
    IoEvent* ev       = nullptr;
    eIoAction act     = eIoAction::None;

    try
    {
        while (!isStopping())
        {
            doConnect();

            _now = std::time(nullptr);
            _timeoutMgr->checkTimeout(_now);

            // Needs to wake up to reconnect. So we should use small
            // milliseconds.
            eventNum = _notifier->waitIoEvents(1000);

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
                            updateTimeout(static_cast<RpcClientConn<Sess>*>(
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
                        // TODO: add to disconnected list.
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

            // Send responses to the handler.
            handleRsp();

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
