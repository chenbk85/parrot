#include "wsPacket.h"

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

    std::unique_ptr<std::vector<char>> WsPacket::toBuffer()
    {
        std::unique_ptr<std::vector<char>> buf(new std::vector<char>());
        return buf;
    }
}

