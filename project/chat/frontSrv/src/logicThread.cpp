#include "ioEvent.h"
#include "epoll.h"
#include "kqueue.h"
#include "codes.h"
#include "simpleEventNotifier.h"
#include "logicThread.h"
#include "threadJob.h"

namespace chat
{
LogicThread::LogicThread()
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
    _reqBindJobHdr = std::bind(&LogicThread::handleReqBind, this, _1, _2);
    _packetJobHdr  = std::bind(&LogicThread::handlePacket, this, _1);
}

void LogicThread::setConfig(const Config *cfg)
{
    _config = cfg;
}

void LogicThread::beforeStart()
{
    _notifier->create();
}

void LogicThread::addJob(std::unique_ptr<LoggerJob>&& job)
{
    _jobListLock.lock();
    _jobList.push_back(std::move(job));
    _jobListLock.unlock();

    _notifier->stopWaiting();
}

void LogicThread::addJob(std::list<std::unique_ptr<Job>>& jobList)
{
    _jobListLock.lock();
    _jobList.splice(_jobList.end(), jobList);
    _jobListLock.unlock();

    _notifier->stopWaiting();
}

void LogicThread::handleReqBind(FrontThread* thread,
                                std::list<SessionPktPair>& pktList)
{
    Session* session = nullptr;
    std::list<std::shared_ptr<const Session*>> sessionList;
    for (auto& sp : pktList)
    {
        session                  = const_cast<Session*>(sp.first.get());
        session->_frontThreadPtr = this;
        sessionList.push_back(std::move(sp.first));
    }

    std::unique_ptr<RspBindJob> rbJob(new RspBindJob());
    rbJob.bind(std::move(sessionList));
}

void LogicThread::handlePacket(std::list<SessionPktPair>& pktList)
{
    for (auto& sp : pktList)
    {
    }
}

void LogicThread::handleJob()
{
    std::list<std::unique_ptr<Job>> jobList;
    _jobListLock.lock();
    jobList = std::move(_jobList);
    _jobListLock.unlock();

    for (auto& j : jobList)
    {
        switch (j->getJobType())
        {
            case eJobType::ReqBind:
            {
                std::unique_ptr<ReqBindJob> tj(
                    static_cast<ReqBindJob*>(j.release()));
                tj->call(_reqBindJobHdr);
            }
            break;

            case eJobType::Packet:
            {
                std::unique_ptr<PacketJob> tj(
                    static_cast<PacketJob*>(j.release()));
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

void LogicThread::run()
{
    IoEvent* ev = nullptr;
    uint32_t ret = 0;
    
    while (!isStopped())
    {
        ret = _notifier->waitIoEvents(-1);

        for (auto i = 0u; i < ret; ++i)
        {
            ev = _notifier->getIoEvent(i);
            ev->handleIoEvent();
        }

        processJob();        
    }
}

void LogicThread::stop()
{
    _notifier->stopWaiting();
    ThreadBase::stop();
}
}
