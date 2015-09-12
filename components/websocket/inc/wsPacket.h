#ifndef __COMPONENT_WEBSOCKET_INC_WSPACKET_H__
#define __COMPONENT_WEBSOCKET_INC_WSPACKET_H__

namespace parrot
{
    class WsPacket
    {
        class Json;

      public:
        enum class ePacketItem
        {
            Route               = 0,            // Packet route.
            Json                = 1,            // Json data.
            Binary              = 2,            // Binary data.
            Version             = 3             // Payload format version.
        };

        enum class eOpCode
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

      public:
        void setJson(std::unique_ptr<Json> &&json);
        void setBinary(const char *bin, uint32_t len);
        void setRoute(int route);

        int getRoute() const;
        std::vector<char> & getBinary();
        std::unique_ptr<Json> & getJson();

      public:
        void parse();

      protected:
        uint32_t                        _lastParsePos;
        std::unique_ptr<Json>           _jsonData;
        std::vector<char>               _binData;
        uint32_t                        _route;
    };
}

#endif
