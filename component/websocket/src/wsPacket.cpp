#include <algorithm>
#include "wsPacket.h"
#include "json.h"
#include "sysHelper.h"
#include "macroFuncs.h"

namespace parrot
{
WsPacket::WsPacket()
    : _opCode(eOpCode::Binary),
      _closeCode(eCodes::WS_NormalClosure),
      _reason(),
      _json(),
      _sysJson(),
      _bin(),
      _payload(),
      _route(0),
      _reqId(0),
      _decoded(false),
      _decodeResult(false)
{
}

bool WsPacket::isRaw() const
{
    return _payload.empty() ? false : true;
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

void WsPacket::setSysJson(std::unique_ptr<Json>&& json)
{
    _sysJson = std::move(json);
}

void WsPacket::setBinary(std::vector<unsigned char> &&bin)
{
    _bin = std::move(bin);
}

void WsPacket::setPacket(eOpCode opCode, std::vector<unsigned char> &&payload)
{
    _opCode = opCode;
    _payload = std::move(payload);

    if (opCode == eOpCode::Binary)
    {
        if (!decodeSysData())
        {
            _decoded = true;
            _decodeResult = false;
        }
    }
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

const std::vector<unsigned char>& WsPacket::getBinary() const
{
    return _bin;
}

const Json* WsPacket::getJson() const
{
    return _json.get();
}

const Json* WsPacket::getSysJson() const
{
    return _sysJson.get();
}

const std::vector<unsigned char>& WsPacket::getPayload() const
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
    // Close packet should contain 2 bytes error code. if greater than 2 bytes
    // then, the remote provided the reason.
    if (_payload.size() < 2)
    {
        _closeCode = eCodes::WS_ProtocolError;
        return false;
    }

    uint16_t code = *reinterpret_cast<uint16_t*>(&_payload[0]);
    code = uniNtohs(code);
    _closeCode = static_cast<eCodes>(code);

    if (_payload.size() > 2)
    {
        _reason = std::string(reinterpret_cast<char*>(&_payload[2]),
                              _payload.size() - 2);
    }
    
    return true;
}

bool WsPacket::decodeItemMeta(std::vector<unsigned char>::const_iterator& it,
                              ePayloadItem& item,
                              uint64_t &itemLen)
{
    item = static_cast<ePayloadItem>(*it++);
    if (it == _payload.end())
    {
        return false;
    }

    itemLen = reinterpret_cast<uint8_t>(*it++);
    if (itemLen == 254)
    {
        if (_payload.end() - it < 2)
        {
            return false;
        }
        itemLen = *reinterpret_cast<const uint16_t*>(&(*it));
        it += 2;

        itemLen = uniNtohs(static_cast<uint16_t>(itemLen));
    }
    else if (itemLen == 255)
    {
        if (_payload.end() - it < 8)
        {
            return false;
        }
        itemLen = *reinterpret_cast<const uint64_t*>(&(*it));
        it += 8;
        itemLen = uniNtohll(static_cast<uint16_t>(itemLen));
    }

    if (_payload.cend() - it < static_cast<int64_t>(itemLen))
    {
        return false;
    }

    return true;
}

bool WsPacket::decodeBinary()
{
    if (_payload.empty())
    {
        return false;
    }

    auto it = _payload.cbegin();    
    if (_route < 254)
    {
        ++it;
    }
    else if (_route >= 254 && _route <= 0xFFFF)
    {
        it += 3;
    }
    else
    {
        it += 9;
    }

    ePayloadItem item = ePayloadItem::None;
    uint64_t itemLen  = 0;

    while (it < _payload.cend())
    {
        if (!decodeItemMeta(it, item, itemLen))
        {
            return false;
        }

        if (item == ePayloadItem::Json)
        {
            _json.reset(new Json());
            if (!_sysJson->parse(reinterpret_cast<const char*>(&(*it)), itemLen))
            {
                return false;
            }
        }
        else if (item == ePayloadItem::Binary)
        {
            std::copy_n(it, itemLen, std::back_inserter(_bin));
        }
        else if (item == ePayloadItem::SysJson)
        {
        }
        else
        {
            // Bad packet.
            return false;
        }

        it += itemLen;
    }

    return false;
}

bool WsPacket::decodeSysData()
{
    if (_payload.empty())
    {
        return false;
    }

    auto it = _payload.cbegin();
    _route = reinterpret_cast<uint8_t>(*it++);
    if (_route == 254)
    {
        if (_payload.size() < 3)
        {
            return false;
        }

        _route = *reinterpret_cast<const uint16_t*>(&(*it));
        it += 2;
    }
    else if (_route == 255)
    {
        if (_payload.size() < 9)
        {
            return false;
        }
        _route = *reinterpret_cast<const uint64_t*>(&(*it));
        it += 8;
    }

    ePayloadItem item = ePayloadItem::None;
    uint64_t itemLen  = 0;

    while (it < _payload.cend())
    {
        if (!decodeItemMeta(it, item, itemLen))
        {
            return false;
        }

        if (item == ePayloadItem::SysJson)
        {
            _sysJson.reset(new Json());
            if (_sysJson->parse(reinterpret_cast<const char*>(&(*it)), itemLen))
            {
                return true;
            }

            return false;
        }

        it += itemLen;
    }

    return false;
}
}
