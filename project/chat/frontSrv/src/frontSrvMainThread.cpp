#include <unordered_map>

#include "frontSrvMainThread.h"
#include "frontSrvConfig.h"
#include "jobHandler.h"
#include "logger.h"

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
    std::unordered_map<void *,parrot::JobHandler*> jobHandlerMap;
    std::vector<parrot::JobHandler*> jobHandlerVec;
    for (auto& t : threads)
    {
        t->setConfig(static_cast<const FrontSrvConfig*>(_config));
        jobHandlerMap[t.get()] = t.get();
        jobHandlerVec.push_back(t.get());
    }
    
    _logicThreadPool.start();

    setFrontThreadDefaultJobHandler(jobHandlerVec);
    setFrontThreadJobHandler(jobHandlerMap);
    LOG_INFO("FrontSrvMainThread::createUserThreads. Done.");
}

void FrontSrvMainThread::stopUserThreads()
{
    _logicThreadPool.destroy();
}
}
