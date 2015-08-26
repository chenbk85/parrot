#ifndef __COMPONENT_WEBSOCKET_INC_WSTRANSLAYER_H__
#define __COMPONENT_WEBSOCKET_INC_WSTRANSLAYER_H__

#include <vector>
#include <cstdint>

namespace parrot
{
    class WsTranslayer
    {
      public:
        WsTranslayer(IoEvent *io);
        ~WsTranslayer();

      public:
        void recvPacket();
        void getPacket();
        bool sendPacket();
        
      private:
        std::vector<char>         _sendVec;
        uint32_t                  _sentLen;

        std::vector<char>         _recvBuf;
        uint32_t                  _needRecvLen;
    };
}



#endif
