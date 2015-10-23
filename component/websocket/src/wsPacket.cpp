#include "wsPacket.h"
#include "json.h"
#include "macroFuncs.h"

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
      _reqId(0),
      _decoded(false),
      _decodeResult(false)
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

void WsPacket::setReqId(uint64_t reqId)
{
    _reqId = reqId;
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

uint64_t WsPacket::getReqId() const
{
    return _reqId;
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

bool WsPacket::decode()
{
    if (_decoded)
    {
        return _decodeResult;
    }

    if (_opCode == eOpCode::Binary)
    {
        _decodeResult = decodeBinary();
    }
    else if (_opCode == eOpCode::Close)
    {
        _decodeResult = decodeClose();
    }
    else
    {
        if (_opCode == eOpCode::Ping || _opCode == eOpCode::Pong)
        {
            _decodeResult = true;
        }

        if (_opCode == eOpCode::Continue || _opCode == eOpCode::Text)
        {
            PARROT_ASSERT(false);
            _decodeResult = false;
        }
    }

    _decoded = true;
    return _decodeResult;
}

bool WsPacket::decodeClose()
{
    return false;
}

bool WsPacket::decodeBinary()
{
    return false;
}
}
