#ifndef __PROJECT_CHAT_FRONTSRV_INC_FRONTSRVCONFIG_H__
#define __PROJECT_CHAT_FRONTSRV_INC_FRONTSRVCONFIG_H__

#include "config.h"

namespace chat
{
struct FrontSrvConfig : public parrot::Config
{
    uint32_t _logicThreadPoolSize = 1;
};
}

#endif
