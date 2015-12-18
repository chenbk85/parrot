#include <iostream>

#include "logger.h"
#include "ioEvent.h"
#include "epoll.h"
#include "kqueue.h"
#include "codes.h"
#include "session.h"
#include "simpleEventNotifier.h"
#include "frontSrvLogicThread.h"
#include "frontThread.h"
#include "wsPacket.h"
#include "threadJob.h"

namespace chat
{
FrontSrvLogicThread::FrontSrvLogicThread()
    : PoolThread(),
      JobHandler(),
      _packetJobHdr(),
      _jobListLock(),
      _jobList(),
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

void FrontSrvLogicThread::beforeStart()
{
    _notifier->create();
}

void FrontSrvLogicThread::addJob(std::unique_ptr<parrot::Job>&& job)
{
    LOG_DEBUG("FrontSrvLogicThread::AddJob.");
    _jobListLock.lock();
    _jobList.push_back(std::move(job));
    _jobListLock.unlock();

    _notifier->stopWaiting();
}

void FrontSrvLogicThread::addJob(
    std::list<std::unique_ptr<parrot::Job>>& jobList)
{
    LOG_DEBUG("FrontSrvLogicThread::AddJob. List size is " << jobList.size()
                                                           << ".");
    _jobListLock.lock();
    _jobList.splice(_jobList.end(), jobList);
    _jobListLock.unlock();

    _notifier->stopWaiting();
}

void FrontSrvLogicThread::handlePacket(
    std::list<parrot::SessionPktPair>& pktList)
{
    std::unordered_map<parrot::JobHandler, std::list<parrot::SessionPktPair>>
        rspMap;

    for (auto& sp : pktList)
    {
        LOG_INFO("FrontSrvLogicThread::handlePacket: session is "
                 << (sp.first)->toString() << ".");
        std::unique_ptr<parrot::WsPacket> pkt(new parrot::WsPacket());
        pkt->setRoute(1);
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
        std::unique_ptr<parrot::PacketJob> job(new parrot::PacketJob());
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
                std::unique_ptr<parrot::PacketJob> tj(
                    static_cast<parrot::PacketJob*>(j.release()));
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
