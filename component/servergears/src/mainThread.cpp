#include "config.h"
#include "wsServerConn.h"
#include "mainThread.h"
#include "threadBase.h"
#include "frontThread.h"
#include "daemon.h"

 namespace parrot
{
MainThread::MainThread(const Config* cfg)
    : _frontListener(),
      _listenerNotifier,
      _frontThreadPtr(new FrontThreadPool()),
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
}

void MainThread::beforeStart()
{
    daemonize();
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

    _frontListener.reset(new Listener<WsServerConn, ConnFactory>(
                             _config->_thisServer._frontPort, _config->_thisServer._frontIp));

    

    while (!Daemon::isShutdown())
    {
        eventNum = _notifier->waitIoEvents(-1);

        for (int i = 0; i != eventNum; ++i)
        {
            
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
