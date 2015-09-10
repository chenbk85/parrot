#ifndef __BASE_SYS_INC_PACKET_H__
#define __BASE_SYS_INC_PACKET_H__

#include <vector>
#include <cstdint>

namespace parrot
{
    class Packet
    {
      public:
        virtual uint32_t getPktLen() const = 0;
        virtual const vector<char> & getPktBuff() const = 0;

        void setRoute(int r) { _route = r; }
        uint32_t getRoute() const { return _route; }

      protected:
        uint32_t                   _route;
    };
}

#endif
