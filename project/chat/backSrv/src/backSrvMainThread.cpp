#include <iostream>
#include <vector>
#include <memory>

#include "backSrvJobProcesser.h"
#include "backSrvMainThread.h"
#include "backSrvConfig.h"
#include "logger.h"
#include "backSrvRpcScheduler.h"

namespace chat
{
std::unique_ptr<BackSrvMainThread> BackSrvMainThread::_instance;

BackSrvMainThread::BackSrvMainThread(const BackSrvConfig* cfg)
    : MainThread<ChatSession, ChatSession>(cfg),
      _logicThreadPool(cfg->_logicThreadPoolSize),
      _config(cfg)
{
}

void BackSrvMainThread::createInstance(const BackSrvConfig* cfg)
{
    _instance.reset(new BackSrvMainThread(cfg));
}

BackSrvMainThread* BackSrvMainThread::getInstance()
{
    return _instance.get();
}

parrot::ThreadPool<parrot::LogicPoolThread>&
BackSrvMainThread::getLogicThreadPool()
{
    return _logicThreadPool;
}

const BackSrvConfig* BackSrvMainThread::getConfig() const
{
    return _config;
}

void BackSrvMainThread::createUserThreads()
{
    _logicThreadPool.create();
    _logicThreadPool.start();
    LOG_INFO("BackSrvMainThread::createUserThreads. Done.");
}

void BackSrvMainThread::beforeStart()
{
    MainThread<ChatSession, ChatSession>::beforeStart();
    BackSrvRpcScheduler::createInstance();

    auto& logicThreadsVec = _logicThreadPool.getThreadPoolVec();
    for (auto& t : logicThreadsVec)
    {
        std::unique_ptr<BackSrvJobProcesser> jp(new BackSrvJobProcesser());
        jp->setLogicThread(t.get());
        t->setJobProcesser(std::move(jp));
    }
}

void BackSrvMainThread::stopUserThreads()
{
    _logicThreadPool.destroy();
}
}
