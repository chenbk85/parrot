#include "backSrvRpcScheduler.h"

namespace chat
{
std::unique_ptr<BackSrvRpcScheduler> BackSrvRpcScheduler::_instance;

void BackSrvRpcScheduler::makeInstance()
{
    _instance.reset(new BackSrvRpcScheduler());
    setInstance(std::move(_instance));
}

parrot::JobHandler*
BackSrvRpcScheduler::getHandler(uint64_t,
                                std::shared_ptr<const parrot::RpcSession>)
{
    return nullptr;
}

parrot::JobHandler* BackSrvRpcScheduler::getOnCloseHandler(
    std::shared_ptr<const parrot::RpcSession>)
{
    return nullptr;
}
}
