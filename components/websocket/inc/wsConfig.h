#ifndef __COMPONENT_WEBSOCKET_INC_WSCONFIG_H__
#define __COMPONENT_WEBSOCKET_INC_WSCONFIG_H__

#include <string>

namespace parrot
{
    struct WsConfig
    {
        std::string                  _host;
        uint32_t                     _maxPacketLen = 1 << 20; // 1MB.
        uint32_t                     _maxHttpHandshake = 1 << 13; // 8Kb.
    };
}

#endif
