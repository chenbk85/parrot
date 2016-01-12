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
      //      TimeoutHandler<WsServerConn<RpcSession>>(),
      JobManager(),
      RpcServerConnManager(this),
      //      ConnHandler<RpcServerConn>(),
      //      _connMap(),
      //      _registeredConnMap(),
      _jobProcesser(),
      _notifier(),
      //      _timeoutMgr(),
      _now(0),
      //      _random(),
      _config(cfg)
{
    init();
}

void RpcServerThread::init()
{
    _jobProcesser.reset(new RpcServerJobProcesser(this));

    createTimeManager(_config->_rpcClientConnTimeout);

#if defined(__linux__)
    _notifier.reset(new Epoll(_config->_neighborSrvMap.size()));
#elif defined(__APPLE__)
    _notifier.reset(new Kqueue(_config->_neighborSrvMap.size()));
#elif defined(_WIN32)
//    _notifier.reset(new
//    SimpleEventNotifier(_config->_frontThreadMaxConnCount));
#endif
    _notifier->create();

    JobManager::setJobProcesser(_jobProcesser.get());
    JobManager::setEventNotifier(_notifier.get());
    RSConnMgr::setEventNotifier(_notifier.get());
}

void RpcServerThread::stop()
{
    ThreadBase::stop();
    _notifier->stopWaiting();
    ThreadBase::join();
    LOG_INFO("RpcServerThread::stop: Done.");
}

void RpcServerThread::addReqPacket(JobManager* mgr,
                                   RpcSrvReqJobParam&& jobParam)
{
    _jobProcesser->createRpcReqJob(mgr, std::move(jobParam));
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

            RSConnMgr::_timeoutMgr->checkTimeout(_now);
            RSConnMgr::addConnToNotifier();

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
                            RSConnMgr::updateTimeout(
                                static_cast<RpcServerConn*>(
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
                        RSConnMgr::removeConn(
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
