#include "config.h"
#include "daemonBase.h"


namespace parrot
{
DaemonBase::DaemonBase(): _shutdownCb(), _config(nullptr)
{
}

void DaemonBase::setConfig(const Config* cfg)
{
    _config = cfg;
}

void DaemonBase::registerShutdownCb(std::function<void()> &&cb)
{
    _shutdownCb = std::move(cb);
}
}
