#include "frontSrvRpcScheduler.h"

namespace chat
{
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
    return nullptr;
}

parrot::JobHandler* FrontSrvRpcScheduler::getOnCloseHandler(
    std::shared_ptr<const parrot::RpcSession>)
{
    return nullptr;
}
}
