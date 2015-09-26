#include "wsPacket.h"
#include "json.h"

namespace parrot {
WsPacket::WsPacket() : _json(), _bin(), _raw(), _route(0) {
}

bool WsPacket::isPacketUndecoded() const {
    return _raw;
}

void WsPacket::setRoute(uint32_t route) {
    _route = route;
}

void WsPacket::setJson(Json &&json) {
    _json = std::move(json);
}

void WsPacket::setBinary(std::vector<char>> &&bin) {
    _bin = std::move(bin);
}

void WsPacket::setRawData(std::vector<char>> &&raw) {
    _raw = std::move(raw);
}

uint32_t WsPacket::getRoute() const {
    return _route;
}

const std::vector<char> &WsPacket::getBinary() const {
    return *(_bin.get());
}

const Json &WsPacket::getJson() const {
    return *(_json.get());
}

const std::vector<char> &WsPacket::getRawData() const {
    return *(_raw.get());
}
}
