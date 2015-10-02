#ifndef __COMPONENTS_WEBSOCKET_INC_WSCONFIG_H__
#define __COMPONENTS_WEBSOCKET_INC_WSCONFIG_H__

#include <string>
#include <cstdint>

namespace parrot
{
struct WsConfig
{
    WsConfig()
    {
        static_assert(_sendBuffLen >= 128, "Send buffer is too small.");
        static_assert(_recvBuffLen >= 128, "Recv buffer is too small.");
        static_assert(_fragmentThreshold >= 128,
                      "Fragment threshlod is too small.");
    }

    std::string _host;
    const static uint32_t _maxPacketLen = (1 << 20);      // 1MB.
    const static uint32_t _maxHttpHandshake = (1 << 13);  // 8KB.
    const static uint32_t _fragmentThreshold = (1 << 14); // 16KB.

    //According to RFC6455, the max header size is 14 bytes.
    const static uint32_t _recvBuffLen = (1 << 14) + 14; // Payload len + max header len.
    const static uint32_t _sendBuffLen = (1 << 14) + 14; // Payload len + max header len.
};
}



#endif
