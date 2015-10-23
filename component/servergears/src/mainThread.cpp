#include "mainThread.h"
#include "daemon.h"

namespace parrot
{
MainThread::MainThread(const Config& cfg) : _config(cfg)
{
}

void MainThread::daemonize()
{
    Daemon::setConfig(&_config);

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
}

void MainThread::beforeStart()
{
    daemonize();
    createThreads();
}

void MainThread::start()
{
    beforeStart();
    run();
    beforeTerminate();
}

void MainThread::run()
{
    while (!Daemon::isShutdown())
    {
    }
}

void MainThread::onStop()
{
}

void MainThread::beforeTerminate()
{
    Daemon::beforeTerminate();
}
}
