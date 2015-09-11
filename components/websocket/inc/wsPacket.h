#ifndef __COMPONENT_WEBSOCKET_INC_WSPACKET_H__
#define __COMPONENT_WEBSOCKET_INC_WSPACKET_H__

namespace parrot
{
    class WsPacket
    {
      public:
        enum ePacketItem
        {
            Route               = 0,
            Json                = 1,
            Binary              = 2
        };

      protected:
        enum eOpCode
        {
            Continue   = 0x0,
            Text       = 0x1,
            Binary     = 0x2,
            // 0x3-7 are reserved. 
            Close      = 0x8,
            Ping       = 0x9,
            Pong       = 0xA
            // 0xB-F are reserved. 
        };

      public:
        WsPacket();
        virtual ~WsPacket();

        

      protected:
        uint32_t                        _lastParsePos;
    };
}

#endif
