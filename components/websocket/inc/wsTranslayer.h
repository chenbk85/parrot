#ifndef __COMPONENT_WEBSOCKET_INC_WSTRANSLAYER_H__
#define __COMPONENT_WEBSOCKET_INC_WSTRANSLAYER_H__

#include <list>
#include <memory>
#include <vector>
#include <cstdint>

#include "codes.h"

namespace parrot
{
    class WsTranslayer
    {
        enum
        {
            SEND_BUFF_LEN = 65536,
            RECV_BUFF_LEN = 65536
        };
        friend class WsParser;

      public:
        WsTranslayer(IoEvent *io);
        ~WsTranslayer();
        WsTranslayer(const WsTranslayer &) = delete;
        WsTranslayer& operator=(const WsTranslayer &) = delete;

      public:
        void recvPacket();
        void getPacket();
        bool sendPacket();
        
      private:
        std::vector<char>         _sendVec;
        uint32_t                  _sentLen;

        std::vector<char>         _recvVec;
        uint32_t                  _needRecvLen;

        IoEvent *                 _io;
    };
}



#endif
