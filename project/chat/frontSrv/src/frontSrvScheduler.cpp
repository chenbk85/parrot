#include "frontSrvScheduler.h"

namespace chat
{
std::unique_ptr<FrontSrvScheduler> FrontSrvScheduler::_instance;

void FrontSrvScheduler::makeInstance()
{
    _instance.reset(new FrontSrvScheduler());
    setInstance(std::move(_instance));
}

parrot::JobHandler*
FrontSrvScheduler::getHandler(uint64_t,
                              std::shared_ptr<const ChatSession>)
{
    return nullptr;
}

parrot::JobHandler*
FrontSrvScheduler::getOnCloseHandler(std::shared_ptr<const ChatSession>)
{
    return nullptr;
}
}
