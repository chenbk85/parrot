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
#include "wsPacket.h"
#include "threadJob.h"

namespace chat
{
FrontSrvLogicThread::FrontSrvLogicThread()
    : PoolThread(),
      JobHandler(),
      _mainThread(nullptr),
      _packetJobHdr(),
#if defined(__linux__)
      _notifier(new parrot::Epoll(1)),
#elif defined(__APPLE__)
      _notifier(new parrot::Kqueue(1)),
#elif defined(_WIN32)
//      _notifier(new parrot::SimpleEventNotifier()),
#endif
      _config(nullptr)
{
    using namespace std::placeholders;
    _packetJobHdr = std::bind(&FrontSrvLogicThread::handlePacket, this, _1);
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
    std::list<parrot::SessionPktPair<ChatSession>>& pktList)
{
    std::unordered_map<parrot::JobHandler*,
                       std::list<parrot::SessionPktPair<ChatSession>>> rspMap;

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

        rspMap[(sp.first)->getFrontJobHdr()].emplace_back(std::move(sp.first),
                                                          std::move(pkt));
    }

    for (auto& kv : rspMap)
    {
        std::unique_ptr<parrot::PacketJob<ChatSession>> job(
            new parrot::PacketJob<ChatSession>());
        job->bind(std::move(kv.second));
        (kv.first)->addJob(std::move(job));
    }
}

void FrontSrvLogicThread::handleJob()
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

            default:
            {
                PARROT_ASSERT(false);
            }
            break;
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

        handleJob();
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
