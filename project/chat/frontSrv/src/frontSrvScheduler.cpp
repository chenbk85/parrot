#include <vector>
#include <memory>
#include <thread>

#include "threadPool.h"
#include "frontSrvScheduler.h"
#include "frontSrvMainThread.h"

namespace chat
{
static thread_local uint8_t gLogicThreadIdx = 0;

void FrontSrvScheduler::createInstance()
{
    _scheduler.reset(new FrontSrvScheduler());
}

parrot::JobHandler*
FrontSrvScheduler::getHandler(uint64_t, std::shared_ptr<const ChatSession>)
{
    auto& logicThreadPool =
        FrontSrvMainThread::getInstance()->getLogicThreadPool();
    auto & threadVec = logicThreadPool.getThreadPoolVec();
    auto hdr =threadVec[gLogicThreadIdx].get();
    
    gLogicThreadIdx = (gLogicThreadIdx + 1) % threadVec.size();
    
    return hdr;
}

parrot::JobHandler*
FrontSrvScheduler::getOnCloseHandler(std::shared_ptr<const ChatSession>)
{
    return nullptr;
}
}
