#include "logger.h"
#include "ioEvent.h"
#include "epoll.h"
#include "kqueue.h"
#include "codes.h"
#include "session.h"
#include "simpleEventNotifier.h"
#include "frontSrvLogicThread.h"
#include "frontThread.h"
#include "threadJob.h"

namespace chat
{
FrontSrvLogicThread::FrontSrvLogicThread()
    : PoolThread(),
      JobHandler(),
      _reqBindJobHdr(),
      _packetJobHdr(),
      _jobListLock(),
      _jobList(),
#if defined(__linux__)
      _notifier(new parrot::Epoll(1)),
#elif defined(__APPLE__)
      _notifier(new parrot::Kqueue(1)),
#elif defined(_WIN32)
      _notifier(new parrot::SimpleEventNotifier()),
#endif
      _config(nullptr)
{
    using namespace std::placeholders;
    _reqBindJobHdr =
        std::bind(&FrontSrvLogicThread::handleReqBind, this, _1, _2);
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
    _jobListLock.lock();
    _jobList.push_back(std::move(job));
    _jobListLock.unlock();

    _notifier->stopWaiting();
}

void FrontSrvLogicThread::addJob(
    std::list<std::unique_ptr<parrot::Job>>& jobList)
{
    _jobListLock.lock();
    _jobList.splice(_jobList.end(), jobList);
    _jobListLock.unlock();

    _notifier->stopWaiting();
}

void FrontSrvLogicThread::handleReqBind(
    parrot::FrontThread* thread, std::list<parrot::SessionPktPair>& pktList)
{
    parrot::Session* session = nullptr;
    std::list<std::shared_ptr<const parrot::Session>> sessionList;
    for (auto& sp : pktList)
    {
        session                  = const_cast<parrot::Session*>(sp.first.get());
        session->_frontThreadPtr = this;
        sessionList.push_back(std::move(sp.first));
    }

    std::unique_ptr<parrot::RspBindJob> rbJob(new parrot::RspBindJob());
    rbJob->bind(std::move(sessionList));
    thread->addJob(std::move(rbJob));
}

void FrontSrvLogicThread::handlePacket(
    std::list<parrot::SessionPktPair>& pktList)
{
    for (auto& sp : pktList)
    {
        LOG_INFO("FrontSrvLogicThread::handlePacket: session is "
                 << (sp.first)->toString() << ".");
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
            case parrot::eJobType::ReqBind:
            {
                std::unique_ptr<parrot::ReqBindJob> tj(
                    static_cast<parrot::ReqBindJob*>(j.release()));
                tj->call(_reqBindJobHdr);
            }
            break;

            case parrot::eJobType::Packet:
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

    
    LOG_INFO("FrontSrvLogicThread::run. Tid is " << std::this_thread::get_id() << ".");
    
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
