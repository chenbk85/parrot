#include "daemon.h"
#include "daemonNix.h"
#include "daemonBase.h"

namespace parrot
{
#ifdef _WIN32
DaemonBase* Daemon::_daemon = nullptr;
#else
DaemonBase* Daemon::_daemon = &DaemonNix::getInstance();
#endif

void Daemon::setConfig(const Config* cfg)
{
    _daemon->setConfig(cfg);
}

void Daemon::daemonize()
{
    _daemon->daemonize();
}

void Daemon::beforeCreateThreads()
{
    _daemon->beforeCreateThreads();
}

void Daemon::afterCreateThreads()
{
    _daemon->afterCreateThreads();
}

bool Daemon::isShutdown()
{
    return _daemon->isShutdown();
}

void Daemon::registerShutdownCb(std::function<void()> &&cb)
{
    _daemon->registerShutdownCb(std::move(cb));
}

void Daemon::beforeTerminate()
{
    _daemon->beforeTerminate();
}
}
