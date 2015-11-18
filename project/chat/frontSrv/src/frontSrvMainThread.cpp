#include <iostream>
#include <unordered_map>
#include <unordered_set>

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
    std::unordered_set<parrot::JobHandler*> jobHandlerSet;
    std::vector<parrot::JobHandler*> jobHandlerVec;
    for (auto& t : threads)
    {
        t->setConfig(static_cast<const FrontSrvConfig*>(_config));
        jobHandlerSet.insert(t.get());
        jobHandlerVec.push_back(t.get());
    }

    _logicThreadPool.start();

    setFrontThreadDefaultJobHandler(jobHandlerVec);
    setFrontThreadJobHandler(jobHandlerSet);
    LOG_INFO("FrontSrvMainThread::createUserThreads. Done.");
}

void FrontSrvMainThread::stopUserThreads()
{
    _logicThreadPool.destroy();
}
}
