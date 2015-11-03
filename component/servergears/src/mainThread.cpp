#include <vector>

#include "config.h"
#include "wsServerConn.h"
#include "mainThread.h"
#include "threadBase.h"
#include "frontThread.h"
#include "daemon.h"
#include "epoll.h"

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
      _config(cfg)
{
    _notifier->create();
}

void MainThread::daemonize()
{
    Daemon::setConfig(_config);

    // Make this process as daemon.
    Daemon::daemonize();

    auto shutdownCb = std::bind(&MainThread::onStop, this);
    Daemon::registerShutdownCb(shutdownCb);
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

void MainThread::setFrontConnHandler(
    std::vector<ConnHandler<WsServerConn>*>&& hdrs)
{
    _connDispatcher->setConnHandler(std::move(hdrs));
}

void MainThread::setFrontConnHandler(
    std::vector<ConnHandler<WsServerConn>*>& hdrs)
{
    _connDispatcher->setConnHandler(hdrs);
}

void MainThread::beforeStart()
{
    daemonize();
    createListenerEvents();
    createThreads();
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

    while (!Daemon::isShutdown())
    {
        eventNum = _notifier->waitIoEvents(-1);
        for (int i = 0; i != eventNum; ++i)
        {
            ev = _notifier->getIoEvent(i);
            ev->handleIoEvent();
        }
    }
}

void MainThread::onStop()
{
}

void MainThread::beforeTerminate()
{
    _frontThreadPool->destroy();
    Daemon::beforeTerminate();
}
}
