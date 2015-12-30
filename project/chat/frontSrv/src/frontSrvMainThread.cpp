#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "frontSrvMainThread.h"
#include "frontSrvConfig.h"
#include "jobHandler.h"
#include "logger.h"
#include "frontSrvScheduler.h"

namespace chat
{
FrontSrvMainThread::FrontSrvMainThread(const FrontSrvConfig* cfg)
    : MainThread<ChatSession, ChatSession>(cfg),
      _logicThreadPool(cfg->_logicThreadPoolSize)
{
}

void FrontSrvMainThread::beforeStart()
{
    MainThread<ChatSession, ChatSession>::beforeStart();
    FrontSrvScheduler::makeInstance();
}

void FrontSrvMainThread::createUserThreads()
{
    _logicThreadPool.create();
    _logicThreadPool.start();
    LOG_INFO("FrontSrvMainThread::createUserThreads. Done.");
}

void FrontSrvMainThread::stopUserThreads()
{
    _logicThreadPool.destroy();
}
}
