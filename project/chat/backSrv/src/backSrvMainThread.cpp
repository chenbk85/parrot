#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "backSrvMainThread.h"
#include "backSrvConfig.h"
#include "jobHandler.h"
#include "logger.h"
#include "backSrvRpcScheduler.h"

namespace chat
{
BackSrvMainThread::BackSrvMainThread(const BackSrvConfig* cfg)
    : MainThread<ChatSession, ChatSession>(cfg), _logicThreadPool(cfg->_logicThreadPoolSize)
{
}

void BackSrvMainThread::beforeStart()
{
    MainThread<ChatSession, ChatSession>::beforeStart();
    BackSrvRpcScheduler::makeInstance();
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
