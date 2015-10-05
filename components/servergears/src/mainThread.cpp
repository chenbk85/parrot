#include "mainThread.h"
#include "daemon.h"

namespace parrot
{
MainThread::MainThread(const Config &cfg): _config(cfg)
{
}

void MainThread::beforeStart()
{
    Daemon::setConfig(_config);
    Daemon::daemonize();
}


}
