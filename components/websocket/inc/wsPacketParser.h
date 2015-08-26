#ifndef __COMPONENT_WEBSOCKET_INC_WSPACKETPARSER_H__
#define __COMPONENT_WEBSOCKET_INC_WSPACKETPARSER_H__

namespace parrot
{
    class WsPacketParser
    {
      public:
        WsPacketParser();

        ~WsPacketParser();

      public:
        void parse();
    };
}

#endif
