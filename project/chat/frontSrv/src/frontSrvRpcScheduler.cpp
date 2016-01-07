#include <vector>
#include <memory>
#include <thread>

#include "threadPool.h"
#include "frontSrvMainThread.h"
#include "frontSrvRpcScheduler.h"

namespace chat
{
static thread_local uint8_t gLogicThreadIdx = 0;

void FrontSrvRpcScheduler::createInstance()
{
    _scheduler.reset(new FrontSrvRpcScheduler());
}

parrot::JobHandler*
FrontSrvRpcScheduler::getHandler(uint64_t,
                                 std::shared_ptr<const parrot::RpcSession>)
{
    auto& logicThreadPool =
        FrontSrvMainThread::getInstance()->getLogicThreadPool();
    auto& threadVec = logicThreadPool.getThreadPoolVec();
    auto hdr        = threadVec[gLogicThreadIdx].get();

    gLogicThreadIdx = (gLogicThreadIdx + 1) % threadVec.size();
    return hdr;
}

parrot::JobHandler* FrontSrvRpcScheduler::getOnCloseHandler(
    std::shared_ptr<const parrot::RpcSession>)
{
    return nullptr;
}
}
