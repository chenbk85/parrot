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
    : MainThread<ChatSession>(cfg), _logicThreadPool(cfg->_logicThreadPoolSize)
{
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
