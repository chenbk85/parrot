#ifndef __COMPONENT_WEBSOCKET_INC_WSPACKET_H__
#define __COMPONENT_WEBSOCKET_INC_WSPACKET_H__

#include <memory>
#include <vector>
#include <cstdint>
#include "json.h"

namespace parrot
{
    class WsPacket
    {
      public:
        enum class ePacketItem
        {
            Route               = 0,            // Packet route.
            Json                = 1,            // Json data.
            Binary              = 2,            // Binary data.
            Version             = 3             // Payload format version.
        };


      public:
        WsPacket();
        ~WsPacket() = default;

      public:
        void setJson(std::unique_ptr<Json> &&json);
        void setBinary(std::unique_ptr<std::vector<char>> &&bin);
        void setRoute(uint32_t route);

        uint32_t getRoute() const;
        const std::vector<char> & getBinary();
        const Json & getJson();

        std::unique_ptr<std::vector<char>> toBuffer();
      public:
//        void parse();

      private:
        std::unique_ptr<Json>                  _jsonData;
        std::unique_ptr<std::vector<char>>     _binData;
        std::unique_ptr<std::vector<char>>     _originData;
        uint32_t                               _route;
    };
}

#endif
