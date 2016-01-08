#include <iostream>

#include "logger.h"
#include "ioEvent.h"
#include "epoll.h"
#include "kqueue.h"
#include "codes.h"
#include "simpleEventNotifier.h"
#include "frontSrvMainThread.h"
#include "frontSrvLogicThread.h"
#include "frontThread.h"
#include "rpcClientThread.h"
#include "wsPacket.h"
#include "threadJob.h"
#include "rpcRequest.h"
#include "macroFuncs.h"

namespace chat
{
FrontSrvLogicThread::FrontSrvLogicThread()
    : PoolThread(),
      JobHandler(),
      _mainThread(nullptr),
#if defined(__linux__)
      _notifier(new parrot::Epoll(1)),
#elif defined(__APPLE__)
      _notifier(new parrot::Kqueue(1)),
#elif defined(_WIN32)
//      _notifier(new parrot::SimpleEventNotifier()),
#endif
      _rpcReqContainer(),
      _pktJobFactory(),
      _hdrJobListMap(),
      _updateSessAckJobHdr(),
      _packetJobHdr(),
      _rpcCliRspJobHdr(),
      _config(nullptr)
{
    using namespace std::placeholders;
    _packetJobHdr = std::bind(&FrontSrvLogicThread::handlePacket, this, _1);
    _updateSessAckJobHdr =
        std::bind(&FrontSrvLogicThread::handleUpdateSessionAck, this, _1);
    _rpcCliRspJobHdr =
        std::bind(&FrontSrvLogicThread::handleRpcResponse, this, _1);
}

void FrontSrvLogicThread::setConfig(const FrontSrvConfig* cfg)
{
    _config = cfg;
}

void FrontSrvLogicThread::setMainThread(FrontSrvMainThread* mainThread)
{
    _mainThread = mainThread;
}

void FrontSrvLogicThread::beforeStart()
{
    _notifier->create();
}

void FrontSrvLogicThread::afterAddJob()
{
    _notifier->stopWaiting();
}

void FrontSrvLogicThread::handlePacket(
    std::list<parrot::PacketJobParam<ChatSession>>& pktList)
{
    for (auto& sp : pktList)
    {
        LOG_INFO("FrontSrvLogicThread::handlePacket: session is "
                 << (sp.first)->toString() << ".");

        auto pkt = (sp.second)->toResponsePkt();
        std::unique_ptr<parrot::Json> json(new parrot::Json());
        json->createRootObject();
        std::string s = "Hello world!";
        json->setValue("/s", s);
        pkt->setJson(std::move(json));

        // _clientPktMap[(sp.first)->getFrontJobHdr()].emplace_back(
        //     std::move(sp.first), std::move(pkt));

        // std::unique_ptr<parrot::RpcRequest<ChatSession>> rpcReq(
        //     new parrot::RpcRequest<ChatSession>(
        //         "backSrv001", sp.first,
        //         std::unique_ptr<parrot::WsPacket>(new parrot::WsPacket()),
        //         this));

        std::unique_ptr<parrot::Job> reqJob(new parrot::RpcRequest<ChatSession>(
            "backSrv001", sp.first,
            std::unique_ptr<parrot::WsPacket>(new parrot::WsPacket()), this));
        _rpcReqContainer.add(_mainThread->getRpcCliThread(), std::move(reqJob));
    }

    // for (auto& kv : _clientPktMap)
    // {
    //     std::unique_ptr<parrot::PacketJob<ChatSession>> job(
    //         new parrot::PacketJob<ChatSession>());
    //     job->bind(std::move(kv.second));
    //     (kv.first)->addJob(std::move(job));
    //     kv.second.clear();
    // }
}

void FrontSrvLogicThread::handleRpcResponse(
    std::list<parrot::RpcCliRspJobParam<ChatSession>>& rspList)
{
    parrot::eCodes code;

    // <eCodes, ChatSession, WsPacket>
    for (auto& p : rspList)
    {
        code = std::get<0>(p);
        if (code != parrot::eCodes::ST_Ok)
        {
            std::system_error e(static_cast<int>(code),
                                parrot::ParrotCategory());
            LOG_WARN(
                "FrontSrvLogicThread::handleRpcResponse: Rpc failed. Err is "
                << e.what() << ". Session is " << std::get<1>(p)->toString()
                << ".");
            continue;
        }

        auto& session = std::get<1>(p);
        _pktJobFactory.add(session->getFrontJobHdr(),
                           parrot::PacketJobParam<ChatSession>(
                               std::move(session), std::move(std::get<2>(p))));
    }
}

void FrontSrvLogicThread::handleUpdateSessionAck(
    std::list<std::shared_ptr<const ChatSession>>&)
{
    // TODO: impl this function.
}

void FrontSrvLogicThread::handleJobs()
{
    std::list<std::unique_ptr<parrot::Job>> jobList;
    _jobListLock.lock();
    jobList = std::move(_jobList);
    _jobListLock.unlock();

    for (auto& j : jobList)
    {
        switch (j->getJobType())
        {
            case parrot::JOB_PACKET:
            {
                std::unique_ptr<parrot::PacketJob<ChatSession>> tj(
                    static_cast<parrot::PacketJob<ChatSession>*>(
                        (j.release())->getDerivedPtr()));
                tj->call(_packetJobHdr);
            }
            break;

            case parrot::JOB_RPC_CLI_RSP:
            {
                std::unique_ptr<parrot::RpcCliRspJob<ChatSession>> tj(
                    static_cast<parrot::RpcCliRspJob<ChatSession>*>(
                        (j.release())->getDerivedPtr()));
                tj->call(_rpcCliRspJobHdr);
            }
            break;

            case parrot::JOB_UPDATE_SESSION_ACK:
            {
                std::unique_ptr<parrot::UpdateSessionAckJob<ChatSession>> tj(
                    static_cast<parrot::UpdateSessionAckJob<ChatSession>*>(
                        (j.release())->getDerivedPtr()));
                tj->call(_updateSessAckJobHdr);
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

void FrontSrvLogicThread::dispatchJobs()
{
    _pktJobFactory.loadJobs(_hdrJobListMap);
    _rpcReqContainer.loadJobsWithoutCreate(_hdrJobListMap);

    for (auto& kv : _hdrJobListMap)
    {
        if (!kv.second.empty())
        {
            (kv.first)->addJob(kv.second);
            kv.second.clear();
        }
    }
}

void FrontSrvLogicThread::run()
{
    parrot::IoEvent* ev = nullptr;
    uint32_t ret        = 0;

    LOG_INFO("FrontSrvLogicThread::run. Tid is " << std::this_thread::get_id()
                                                 << ".");

    while (!isStopping())
    {
        ret = _notifier->waitIoEvents(-1);

        for (auto i = 0u; i < ret; ++i)
        {
            ev = _notifier->getIoEvent(i);
            ev->handleIoEvent();
        }

        handleJobs();
        dispatchJobs();
    }
}

void FrontSrvLogicThread::stop()
{
    parrot::ThreadBase::stop();
    _notifier->stopWaiting();
    parrot::ThreadBase::join();
    LOG_INFO("FrontSrvLogicThread::stop: Done.");
}
}
