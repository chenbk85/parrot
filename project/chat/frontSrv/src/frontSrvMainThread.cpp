#include "frontSrvMainThread.h"
#include "frontSrvConfig.h"

namespace chat
{
FrontSrvMainThread::FrontSrvMainThread(const FrontSrvConfig* cfg)
    : MainThread(cfg), _logicThreadPool(cfg->_logicThreadPoolSize)
{
}

void FrontSrvMainThread::createUserThreads()
{
    _logicThreadPool.create();
    auto& threads = _logicThreadPool.getThreadPoolVec();
    std::vector<ConnHandler*> connHandlerVec;
    for (auto& t : threads)
    {
        t->setConfig(static_cast<FrontSrvConfig*>(_config));
        connHandlerVec.push_back(t.get());
    }
    _logicThreadPool.start();

    setFrontConnHandler(std::move(connHandlerVec));
}
}
