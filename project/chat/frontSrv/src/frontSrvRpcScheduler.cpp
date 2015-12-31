#include <vector>
#include <memory>
#include <thread>

#include "threadPool.h"
#include "frontSrvMainThread.h"
#include "frontSrvRpcScheduler.h"

namespace chat
{
static thread_local uint8_t gLogicThreadIdx = 0;
std::unique_ptr<FrontSrvRpcScheduler> FrontSrvRpcScheduler::_instance;

void FrontSrvRpcScheduler::makeInstance()
{
    _instance.reset(new FrontSrvRpcScheduler());
    setInstance(std::move(_instance));
}

parrot::JobHandler*
FrontSrvRpcScheduler::getHandler(uint64_t,
                                 std::shared_ptr<const parrot::RpcSession>)
{
    auto& logicThreadPool =
        FrontSrvMainThread::getInstance()->getLogicThreadPool();
    auto & threadVec = logicThreadPool.getThreadPoolVec();
    auto hdr =threadVec[gLogicThreadIdx].get();
    
    gLogicThreadIdx = (gLogicThreadIdx + 1) % threadVec.size();    
    return hdr;    
}

parrot::JobHandler* FrontSrvRpcScheduler::getOnCloseHandler(
    std::shared_ptr<const parrot::RpcSession>)
{
    return nullptr;
}
}
