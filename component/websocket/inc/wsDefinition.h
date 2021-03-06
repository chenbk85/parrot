#ifndef __COMPONENT_WEBSOCKET_INC_WSDEFINITION_H__
#define __COMPONENT_WEBSOCKET_INC_WSDEFINITION_H__

#include <cstdint>

namespace parrot
{
enum class eOpCode : uint8_t
{
    Continue = 0x0,
    Text     = 0x1,
    Binary   = 0x2,
    // 0x3-7 are reserved.
    Close = 0x8,
    Ping  = 0x9,
    Pong  = 0xA
    // 0xB-F are reserved.
};

enum class ePayloadItem : uint8_t
{
    None    = 0,
    SysJson = 1,
    Json    = 2,
    Binary  = 3
};

enum class ePacketType : uint8_t
{
    Notify  = 1,
    Request = 2,
    Push    = 3
};
}

#endif
