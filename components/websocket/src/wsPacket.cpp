#include "wsPacket.h"
#include "json.h"

namespace parrot
{
WsPacket::WsPacket()
    : _opCode(eOpCode::Binary),
      _closeCode(eCodes::WS_NormalClosure),
      _reason(),
      _json(),
      _bin(),
      _payload(),
      _route(0),
      _decoded(false)
{
}

bool WsPacket::isPacketUndecoded() const
{
    return _decoded;
}

bool WsPacket::isControl() const
{
    return _opCode != eOpCode::Binary;
}

void WsPacket::setRoute(uint64_t route)
{
    _route = route;
}

void WsPacket::setJson(std::unique_ptr<Json>&& json)
{
    _json = std::move(json);
}

void WsPacket::setBinary(std::vector<char> &&bin)
{
    _bin = std::move(bin);
}

void WsPacket::setPacket(eOpCode opCode, std::vector<char> &&payload)
{
    _opCode = opCode;
    _payload = std::move(payload);
}

void WsPacket::setOpCode(eOpCode opCode)
{
    _opCode = opCode;
}

void WsPacket::setClose(eCodes code, std::string&& reason)
{
    _opCode = eOpCode::Close;
    _closeCode = code;
    _reason = std::move(reason);
}

eOpCode WsPacket::getOpCode() const
{
    return _opCode;
}

eCodes WsPacket::getCloseCode() const
{
    return _closeCode;
}

const std::string& WsPacket::getCloseReason() const
{
    return _reason;
}

uint64_t WsPacket::getRoute() const
{
    return _route;
}

const std::vector<char>& WsPacket::getBinary() const
{
    return _bin;
}

const Json& WsPacket::getJson() const
{
    return *(_json.get());
}

const std::vector<char>& WsPacket::getPayload() const
{
    return _payload;
}
}
