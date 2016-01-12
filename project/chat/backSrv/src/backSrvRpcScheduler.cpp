#include "backSrvMainThread.h"
#include "backSrvRpcScheduler.h"

namespace chat
{
static thread_local uint8_t gLogicThreadIdx = 0;

void BackSrvRpcScheduler::createInstance()
{
    _scheduler.reset(new BackSrvRpcScheduler());
}

parrot::JobManager*
BackSrvRpcScheduler::getJobManager(uint64_t,
                                   std::shared_ptr<const parrot::RpcSession>)
{
    auto& logicThreadPool =
        BackSrvMainThread::getInstance()->getLogicThreadPool();
    auto& threadVec = logicThreadPool.getThreadPoolVec();
    auto hdr        = threadVec[gLogicThreadIdx].get();

    gLogicThreadIdx = (gLogicThreadIdx + 1) % threadVec.size();

    return hdr;
}

parrot::JobManager* BackSrvRpcScheduler::getOnCloseJobManager(
    std::shared_ptr<const parrot::RpcSession>)
{
    return nullptr;
}
}
