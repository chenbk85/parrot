#include "backSrvMainThread.h"
#include "backSrvRpcScheduler.h"

namespace chat
{
static thread_local uint8_t gLogicThreadIdx = 0;

std::unique_ptr<BackSrvRpcScheduler> BackSrvRpcScheduler::_instance;

void BackSrvRpcScheduler::createInstance()
{
    _instance.reset(new BackSrvRpcScheduler());
    setInstance(std::move(_instance));
}

BackSrvRpcScheduler* BackSrvRpcScheduler::getInstance()
{
    return _instance.get();
}

parrot::JobHandler*
BackSrvRpcScheduler::getHandler(uint64_t,
                                std::shared_ptr<const parrot::RpcSession>)
{
    auto& logicThreadPool =
        BackSrvMainThread::getInstance()->getLogicThreadPool();
    auto & threadVec = logicThreadPool.getThreadPoolVec();
    auto hdr =threadVec[gLogicThreadIdx].get();

    gLogicThreadIdx = (gLogicThreadIdx + 1) % threadVec.size();    
    
    return hdr;
}

parrot::JobHandler* BackSrvRpcScheduler::getOnCloseHandler(
    std::shared_ptr<const parrot::RpcSession>)
{
    return nullptr;
}
}
