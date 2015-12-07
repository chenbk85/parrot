#ifndef __COMPONENT_WEBSOCKET_INC_WSDEFINITION_H__
#define __COMPONENT_WEBSOCKET_INC_WSDEFINITION_H__

namespace parrot
{
enum class eOpCode
{
    Continue = 0x0,
    Text = 0x1,
    Binary = 0x2,
    // 0x3-7 are reserved.
    Close = 0x8,
    Ping = 0x9,
    Pong = 0xA
    // 0xB-F are reserved.
};

enum class ePayloadItem : uint8_t
{
    None,
    SysJson = 1,
    Json = 2,
    Binary = 3
};
}

#endif
