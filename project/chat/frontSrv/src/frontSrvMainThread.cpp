#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "logicThread.h"
#include "jobProcesser.h"
#include "frontSrvJobProcesser.h"
#include "frontSrvMainThread.h"
#include "frontSrvConfig.h"
#include "jobHandler.h"
#include "logger.h"
#include "frontSrvScheduler.h"
#include "frontSrvRpcScheduler.h"

namespace chat
{
std::unique_ptr<FrontSrvMainThread> FrontSrvMainThread::_instance;

FrontSrvMainThread::FrontSrvMainThread(const FrontSrvConfig* cfg)
    : MainThread<ChatSession, ChatSession>(cfg),
      _config(cfg),
      _logicThreadPool(cfg->_logicThreadPoolSize)
{
}

void FrontSrvMainThread::createInstance(const FrontSrvConfig* cfg)
{
    _instance.reset(new FrontSrvMainThread(cfg));
}

FrontSrvMainThread* FrontSrvMainThread::getInstance()
{
    return _instance.get();
}

parrot::ThreadPool<parrot::LogicPoolThread>&
FrontSrvMainThread::getLogicThreadPool()
{
    return _logicThreadPool;
}

const FrontSrvConfig* FrontSrvMainThread::getConfig() const
{
    return _config;
}

void FrontSrvMainThread::createUserThreads()
{
    _logicThreadPool.create();
    _logicThreadPool.start();
    LOG_INFO("FrontSrvMainThread::createUserThreads. Done.");
}

void FrontSrvMainThread::beforeStart()
{
    MainThread<ChatSession, ChatSession>::beforeStart();
    FrontSrvScheduler::createInstance();
    FrontSrvRpcScheduler::createInstance();

    auto& logicThreadsVec = _logicThreadPool.getThreadPoolVec();
    for (auto& t : logicThreadsVec)
    {
        std::unique_ptr<FrontSrvJobProcesser> jp(new FrontSrvJobProcesser());
        jp->setLogicThread(t.get());
        t->setJobProcesser(std::move(jp));
    }
}

void FrontSrvMainThread::stopUserThreads()
{
    _logicThreadPool.destroy();
}
}
