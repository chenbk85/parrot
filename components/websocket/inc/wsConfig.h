#ifndef __COMPONENT_WEBSOCKET_INC_WSCONFIG_H__
#define __COMPONENT_WEBSOCKET_INC_WSCONFIG_H__

#include <string>
#include <cstdint>

namespace parrot
{
    struct WsConfig
    {
        std::string            _host;
        uint32_t               _maxPacketLen      = 1 << 20;      // 1MB.
        uint32_t               _maxHttpHandshake  = 1 << 13;      // 8KB.
        uint32_t               _fragmentThreshold = 1 << 14;      // 16KB.

        // Receiving buffer should at least holds one packet. If payload is 
        // no more than 125 bytes, the header length is 2. If payload is 
        // greater than 125 bytes, and no more than 64KB, the header length
        // is 2 + 2 + 4 bytes. Else, the header is 2 + 8 + 4 bytes.
        // Normally, we send payload which is more than 125 bytes and less 
        // than 64KB.
        uint32_t               _recvBuffLen       = (1 << 14) + 8;  // Payload len + header len.

        // Sending buffer should at least holds one packet. But server side
        // doesn't need to mask data. So if the payload is more than 125 bytes
        // and no more than 64KB, the header will 4 bytes.
        uint32_t               _sendBuffLen       = (1 << 14) + 4;  // Payload len + header len.
    };
}

#endif
