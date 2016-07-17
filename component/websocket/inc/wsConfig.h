#ifndef __COMPONENT_WEBSOCKET_INC_WSCONFIG_H__
#define __COMPONENT_WEBSOCKET_INC_WSCONFIG_H__

#include <cstdint>
#include <string>

namespace parrot
{
struct WsConfig
{
    WsConfig()
    {
        static_assert(_sendBuffLen >= 256, "Send buffer is too small.");
        static_assert(_recvBuffLen >= 256, "Recv buffer is too small.");
    }

    const static uint64_t _maxPayloadLen    = 256;
    const static uint64_t _maxPacketLen     = (1 << 20); //> 1MB.
    const static uint32_t _maxHttpHandshake = 512;       //(1 << 13);  // 8KB.

    /* According to RFC6455, the max header size is 14 bytes. The WsTranslayer
     * can send multiple WsPacket at one time if the buffer is large enough.
     * So if the _sendBufferLen is a big number,
     */
    const static uint64_t _recvBuffLen = 256;
    const static uint64_t _sendBuffLen = 65546;
};
}

#endif
