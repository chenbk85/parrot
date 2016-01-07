#include <iostream>

#include "logger.h"
#include "ioEvent.h"
#include "epoll.h"
#include "kqueue.h"
#include "codes.h"
#include "simpleEventNotifier.h"
#include "backSrvLogicThread.h"
#include "backSrvMainThread.h"
#include "wsPacket.h"
#include "threadJob.h"
#include "rpcSession.h"

namespace chat
{
BackSrvLogicThread::BackSrvLogicThread()
    : PoolThread(),
      JobHandler(),
      _mainThread(nullptr),
      _reqPktJobHdr(),
#if defined(__linux__)
      _notifier(new parrot::Epoll(1)),
#elif defined(__APPLE__)
      _notifier(new parrot::Kqueue(1)),
#elif defined(_WIN32)
//      _notifier(new parrot::SimpleEventNotifier()),
#endif
      _rpcRspList(),
      _config(nullptr)
{
    using namespace std::placeholders;
    _reqPktJobHdr = std::bind(&BackSrvLogicThread::handleRpcReq, this, _1);
}

void BackSrvLogicThread::setConfig(const BackSrvConfig* cfg)
{
    _config = cfg;
}

void BackSrvLogicThread::setMainThread(BackSrvMainThread* mainThread)
{
    _mainThread = mainThread;
}

void BackSrvLogicThread::beforeStart()
{
    _notifier->create();
}

void BackSrvLogicThread::afterAddJob()
{
    _notifier->stopWaiting();
}

void BackSrvLogicThread::handleRpcReq(PktList& pktList)
{
    // tuple<RpcSession, CliSessionJson, WsPacket>
    for (auto& p : pktList)
    {
        auto& pkt = std::get<2>(p);
        _rpcRspList.emplace_back(std::move(std::get<0>(p)), std::move(pkt));
    }

    std::unique_ptr<parrot::RpcSrvRspJob> rspJob(new parrot::RpcSrvRspJob());
    rspJob->bind(std::move(_rpcRspList));
    _mainThread->getRpcSrvThread()->addJob(std::move(rspJob));
    _rpcRspList.clear();
}

void BackSrvLogicThread::handleJob()
{
    std::list<std::unique_ptr<parrot::Job>> jobList;
    _jobListLock.lock();
    jobList = std::move(_jobList);
    _jobListLock.unlock();

    for (auto& j : jobList)
    {
        switch (j->getJobType())
        {
            case parrot::JOB_RPC_SRV_REQ:
            {
                std::unique_ptr<parrot::RpcSrvReqJob> tj(
                    static_cast<parrot::RpcSrvReqJob*>(
                        (j.release())->getDerivedPtr()));
                tj->call(_reqPktJobHdr);
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

void BackSrvLogicThread::run()
{
    parrot::IoEvent* ev = nullptr;
    uint32_t ret        = 0;

    LOG_INFO("BackSrvLogicThread::run. Tid is " << std::this_thread::get_id()
                                                << ".");

    while (!isStopping())
    {
        ret = _notifier->waitIoEvents(-1);

        for (auto i = 0u; i < ret; ++i)
        {
            ev = _notifier->getIoEvent(i);
            ev->handleIoEvent();
        }

        handleJob();
    }
}

void BackSrvLogicThread::stop()
{
    parrot::ThreadBase::stop();
    _notifier->stopWaiting();
    parrot::ThreadBase::join();
    LOG_INFO("BackSrvLogicThread::stop: Done.");
}
}
