#include <vector>
#include <thread>

#include "config.h"
#include "wsServerConn.h"
#include "mainThread.h"
#include "threadBase.h"
#include "frontThread.h"
#include "daemon.h"
#include "epoll.h"
#include "logger.h"
#include "connFactory.h"

namespace parrot
{
MainThread::MainThread(const Config* cfg)
    : _connDispatcher(new FrontConnDispatcher(cfg->_thisServer._frontPort,
                                              cfg->_thisServer._frontIp)),
#if defined(__linux__)
      _notifier(new Epoll(100)),
#elif defined(__APPLE__)
      _notifier(new Kqueue(100)),
#elif defined(_WIN32)
//      _notifier(new SimpleEventNotifier()),
#endif
      _frontThreadPool(new FrontThreadPool(cfg->_frontThreadMaxConnCount)),
      _wsConfig(new WsConfig()),
      _config(cfg)
{
}

void MainThread::daemonize()
{
    Daemon::setConfig(_config);

    // Make this process as daemon.
    Daemon::daemonize();

    auto shutdownCb = std::bind(&MainThread::onStop, this);
    Daemon::registerShutdownCb(shutdownCb);
}

void MainThread::beforeStart()
{
    daemonize();

    parrot::Logger* logger = parrot::Logger::instance();
    logger->setConfig(_config);
    logger->start();

    auto & sid = _config->_thisServer._serverId;
    LOG_INFO("*************************************************************");
    LOG_INFO("*-->  Service "<< sid <<" started.                           ");
    LOG_INFO("*************************************************************");

    _notifier->create();

    _wsConfig->_host = "10.24.100.202";
    ConnFactory<WsServerConn, WsConfig>::getInstance()->setConfig(
        _wsConfig.get());

    createListenerEvents();
    createThreads();
}

void MainThread::createThreads()
{
    Daemon::beforeCreateThreads();
    createSysThreads();
    createUserThreads();
    Daemon::afterCreateThreads();
}

void MainThread::createSysThreads()
{
    _frontThreadPool->setCount(_config->_frontThreadPoolSize);
    _frontThreadPool->create();

    std::vector<ConnHandler<WsServerConn>*> vec;
    auto& threadVec = _frontThreadPool->getThreadPoolVec();
    for (auto& t : threadVec)
    {
        t->updateByConfig(_config);
        vec.push_back(t.get());
    }
    _connDispatcher->setConnHandler(std::move(vec));
}

void MainThread::createListenerEvents()
{
    auto& thisServer = _config->_thisServer;
    _connDispatcher.reset(
        new FrontConnDispatcher(thisServer._frontPort, thisServer._frontIp));

    _connDispatcher->setNextAction(eIoAction::Read);
    _connDispatcher->startListen();
    _notifier->addEvent(_connDispatcher.get());
}

void MainThread::setFrontThreadDefaultJobHandler(std::vector<JobHandler*>& hdrs)
{
    auto& threadVec = _frontThreadPool->getThreadPoolVec();
    for (auto& t : threadVec)
    {
        t->setDefaultJobHdr(hdrs);
    }
}

void MainThread::setFrontThreadJobHandler(
    std::unordered_map<void*, JobHandler*>& hdrs)
{
    auto& threadVec = _frontThreadPool->getThreadPoolVec();
    for (auto& t : threadVec)
    {
        t->setJobHdr(hdrs);
    }
}

void MainThread::start()
{
    beforeStart();
    _frontThreadPool->start();
    run();
    beforeTerminate();
}

void MainThread::run()
{
    int eventNum = 0;
    IoEvent* ev  = nullptr;

    LOG_INFO("MainThread::run: before run.");
    
    while (!Daemon::isShutdown())
    {
        eventNum = _notifier->waitIoEvents(-1);
        LOG_INFO("MainThread::run: Event num is " << eventNum << ".");
        for (int i = 0; i != eventNum; ++i)
        {
            ev = _notifier->getIoEvent(i);
            ev->handleIoEvent();
        }
    }
}

void MainThread::onStop()
{
    LOG_INFO("MainThread::onStop.");
    _notifier->stopWaiting();
}

void MainThread::stopSysThreads()
{
    _frontThreadPool->destroy();
}

void MainThread::stopThreads()
{
    stopUserThreads();
    stopSysThreads();
}

void MainThread::beforeTerminate()
{
    LOG_INFO("MainThread::beforeTerminate.");
    stopThreads();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    parrot::Logger::instance()->stop();
    Daemon::beforeTerminate();
}
}
