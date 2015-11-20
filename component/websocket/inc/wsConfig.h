#ifndef __COMPONENT_WEBSOCKET_INC_WSCONFIG_H__
#define __COMPONENT_WEBSOCKET_INC_WSCONFIG_H__

#include <string>
#include <cstdint>

namespace parrot
{
struct WsConfig
{
    WsConfig()
    {
        static_assert(_sendBuffLen >= 256, "Send buffer is too small.");
        static_assert(_recvBuffLen >= 256, "Recv buffer is too small.");
    }

    std::string _host;
    const static uint32_t _maxPacketLen = (1 << 20);      // 1MB.
    const static uint32_t _maxHttpHandshake = 512; //(1 << 13);  // 8KB.

    //According to RFC6455, the max header size is 14 bytes.
    const static uint32_t _recvBuffLen = 256;//(1 << 14) + 14; // Payload len + max header len.
    const static uint32_t _sendBuffLen = 65546;//(1 << 14) + 14; // Payload len + max header len.
};
}



#endif
