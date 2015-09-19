#include "wsPacket.h"
#include "json.h"

namespace parrot
{
    WsPacket::WsPacket():
        _jsonData(),
        _binData(),
        _route(0)
    {
    }

    void WsPacket::setJson(std::unique_ptr<Json> &&json)
    {
        _jsonData = std::move(json);
    }

    void WsPacket::setBinary(std::unique_ptr<std::vector<char>> &&bin)
    {
        _binData = std::move(bin);
    }

    void WsPacket::setRoute(uint32_t route)
    {
        _route = route;
    }

    uint32_t WsPacket::getRoute() const
    {
        return _route;
    }

    const std::vector<char> & WsPacket::getBinary()
    {
        return *(_binData.get());
    }

    const Json& WsPacket::getJson()
    {
        return *(_jsonData.get());
    }

    void WsPacket::toWsDataFrame(
        std::vector<char> &buff)
    {
        if (_originData)
        {
            std::copy(_originData.begin(), _originData.end(), 
                      std::back_inserter(buff));
            return;
        }

        std::string jsonStr;
        uint32_t len = 0;
        uint32_t pktLen = 2;

        if (_jsonData)
        {
            jsonStr = std::move(_jsonData->toString());
            len += jsonStr.size();
            
            
        }

        if (_binData)
        {
            len += _binData->size();
        }

        buf->reserve((len  + 64 - 1) / 64 * 64);
            
        return buf;
    }

    uint32_t WsPacket::getPacketLen(uint32_t jsonLen, uint32_t binLen)
    {
        uint32_t pktLen = getRouteLen();
        pktLen += getDataLen(jsonLen);
        pktLen += getDataLen(binLen);

        // If we implement a websocket client, we need to add 4 bytes as
        // mask key.

        if (pktLen < 126)
        {
            pktLen += 2;       // 2 bytes header.
        }
        else if (pktLen >= 126 && pktLen < 65536)
        {
            pktLen += (2 + 2); // 4 bytes header includes 2 bytes length.
        }
        else
        {
            pktLen += (2 + 8); // 10 bytes header includes 8 bytes length.
        }

        return pktLen;
    }

    uint8_t WsPacket::getRouteLen()
    {
        if (_route < 254)
        {
            return 1;
        }
        else if (_route >= 254  && _route < 65536)
        {
            return 3;
        }
        else
        {
            return 5;
        }
    }

    uint32_t WsPacket::getDataLen(uint32_t len)
    {
        if (len == 0)
        {
            return 0;
        }
        else if (len < 254)
        {
            return (1 + 1 + len);        // 1 byte type, 1 byte length 
                                         // and the length of json.
        }
        else if (len >= 254 && len < 65536)
        {
            return (1 + 3 + len);        // 1 byte type, 3 byte length 
                                         // and the length of json.            
        }
        else
        {
            return (1 + 9 + len);        // 1 byte type, 9 byte length 
                                         // and the length of json.
        }
    }
}

