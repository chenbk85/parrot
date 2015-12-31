#include <iostream>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "backSrvMainThread.h"
#include "backSrvConfig.h"
#include "jobHandler.h"
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

parrot::ThreadPool<BackSrvLogicThread>&
BackSrvMainThread::getLogicThreadPool()
{
    return _logicThreadPool;
}

const BackSrvConfig * BackSrvMainThread::getConfig() const
{
    return _config;
}

void BackSrvMainThread::beforeStart()
{
    MainThread<ChatSession, ChatSession>::beforeStart();
    BackSrvRpcScheduler::createInstance();
    
    auto& logicThreadsVec = _logicThreadPool.getThreadPoolVec();
    for (auto& t : logicThreadsVec)
    {
        t->setConfig(_config);
        t->setMainThread(this);
    }
}

void BackSrvMainThread::createUserThreads()
{
    _logicThreadPool.create();
    _logicThreadPool.start();
    LOG_INFO("BackSrvMainThread::createUserThreads. Done.");
}

void BackSrvMainThread::stopUserThreads()
{
    _logicThreadPool.destroy();
}
}
