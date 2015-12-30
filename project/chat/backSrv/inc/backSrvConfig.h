#ifndef __PROJECT_CHAT_BACKSRV_INC_BACKSRVCONFIG_H__
#define __PROJECT_CHAT_BACKSRV_INC_BACKSRVCONFIG_H__

#include "config.h"

namespace chat
{
struct BackSrvConfig : public parrot::Config
{
    uint32_t _logicThreadPoolSize = 1;
};
}

#endif
