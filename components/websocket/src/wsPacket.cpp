#include "wsPacket.h"
#include "json.h"

namespace parrot
{
WsPacket::WsPacket()
    : _opCode(eOpCode::Binary),
      _closeCode(eCodes::NormalClosure),
      _reason(),
      _json(),
      _bin(),
      _raw(),
      _route(0)
{
}

bool WsPacket::isPacketUndecoded() const
{
    return _raw;
}

bool WsPacket::isControl() const
{
    return _opCode == eOpCode::Binary;
}

void WsPacket::setRoute(uint64_t route)
{
    _route = route;
}

void WsPacket::setJson(Json&& json)
{
    _json = std::move(json);
}

void WsPacket::setBinary(std::vector<char>> &&bin)
{
    _bin = std::move(bin);
}

void WsPacket::setRawData(std::vector<char>> &&raw)
{
    _raw = std::move(raw);
}

void WsPacket::setOpCode(eOpCode opCode)
{
    _opCode = opCode;
}

void WsPacket::setClose(eCodes code, std::string&& reason)
{
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

const string& WsPacket::getCloseReason() const
{
    return _reason;
}

uint64_t WsPacket::getRoute() const
{
    return _route;
}

const std::vector<char>& WsPacket::getBinary() const
{
    return *(_bin.get());
}

const Json& WsPacket::getJson() const
{
    return *(_json.get());
}

const std::vector<char>& WsPacket::getRawData() const
{
    return *(_raw.get());
}
}
