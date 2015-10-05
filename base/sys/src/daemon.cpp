#include "daemon.h"
#include "daemonNix.h"

namespace parrot
{
#ifdef _WIN32
Daemon* Daemon::_daemon = nullptr;
#else
Daemon* Daemon::_daemon = &DaemonNix::getInstance();
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

void Daemon::beforeTerminate()
{
    _daemon->beforeTerminate();
}
}
