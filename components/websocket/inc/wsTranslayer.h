#ifndef __COMPONENT_WEBSOCKET_INC_WSTRANSLAYER_H__
#define __COMPONENT_WEBSOCKET_INC_WSTRANSLAYER_H__

#include <vector>
#include <cstdint>

namespace parrot
{
    class WsTranslayer
    {
        enum
        {
            SEND_BUFF_LEN = 65536,
            RECV_BUFF_LEN = 65536
        };

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
        uint32_t                  _rcvdLen;
        uint32_t                  _needRecvLen;

        IoEvent *                 _io;
    };
}



#endif
