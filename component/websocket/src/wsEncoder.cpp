#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "json.h"
#include "logger.h"
#include "macroFuncs.h"
#include "mtRandom.h"
#include "sysHelper.h"
#include "wsConfig.h"
#include "wsDefinition.h"
#include "wsEncoder.h"
#include "wsPacket.h"
#include "wsTranslayer.h"

namespace parrot
{
WsEncoder::WsEncoder(WsTranslayer& trans)
    : _writeState(eWriteState::None),
      _prevWriteState(eWriteState::None),
      _headerLen(0),
      _payloadLen(0),
      _payloadEncodedLen(0),
      _encodedLen(0),
      _totalLen(0),
      _fragmented(false),
      _firstPacket(true),
      _sendVec(trans._sendVec),
      _needSendLen(trans._needSendLen),
      _pktList(trans._pktList),
      _currPkt(),
      _config(trans._config),
      _needMask(trans._needSendMasked),
      _random(*(trans._random)),
      _sysJsonStr(),
      _jsonStr(),
      _maskingKey(0),
      _maskKeyIdx(0),
      _metaData(9), // Just need 9 bytes space.
      _headerVec(14),
      _currPtr(nullptr),
      _maskBeginPtr(nullptr),
      _sendVecEndPtr(&(*_sendVec.begin()) + _sendVec.capacity()),
      _srcPtr(nullptr),
      _srcEndPtr(nullptr)
{
    // Clear default value.
    _metaData.clear();
}

eCodes WsEncoder::loadBuff()
{
    eCodes res;

    while (!_pktList.empty())
    {
        if (!_currPkt)
        {
            _currPkt = std::move(*_pktList.begin());
            _pktList.pop_front();
        }

        res = encode();

        // Reset mask begin pointer. The packet was encoded, or the buffer
        // is full, we need to send the packet next time.
        _maskBeginPtr = nullptr;

        if (res == eCodes::ST_BufferFull)
        {
            _needSendLen = _currPtr - &(*_sendVec.begin());
            _currPtr     = &(*_sendVec.begin());
            return eCodes::ST_Ok;
        }
        else if (res == eCodes::ST_Complete)
        {
            _writeState = eWriteState::None;
            _srcPtr     = nullptr;
            _srcEndPtr  = nullptr;

            _sysJsonStr.clear();
            _jsonStr.clear();
            _currPkt.reset();
        }
        else
        {
            PARROT_ASSERT(false);
        }
    }

    // If here, no packet left in pkt list.
    return eCodes::ST_Complete;
}

eCodes WsEncoder::encode()
{
    PARROT_ASSERT(_currPkt.get());

    eCodes res    = eCodes::ST_Ok;
    auto   opCode = _currPkt->getOpCode();

    if (opCode == eOpCode::Binary)
    {
        // Encode binary data.
        res = encodeDataPacket();
    }
    else if (opCode == eOpCode::Close)
    {
        // Encode close packet.
        res = encodeClosePacket();
    }
    else if (opCode == eOpCode::Ping || opCode == eOpCode::Pong)
    {
        // Encode heartbeat packet.
        res = encodePingPong();
    }
    else
    {
        PARROT_ASSERT(false);
    }

    return res;
}

eCodes WsEncoder::encodePingPong()
{
    // According to the RFC6455, the heartbeat may have 'application data'.
    // But pong must have the extact same data of ping. But the RFC doesn't
    // point out what the data will be. So we use binary if the payload exists.
    // And ping pong packets must not be fragmented.

    switch (_writeState)
    {
        case eWriteState::None:
        {
            auto payloadLen = _currPkt->getBinary().size();
            // No fragment. Up layer should check the length.
            PARROT_ASSERT(payloadLen <= WsConfig::_maxPayloadLen);

            // Compute _headerLen and _payloadLen.
            computeComponentLen(payloadLen);

            _writeState = eWriteState::Header;
            if ((_sendVecEndPtr - _currPtr) < _headerLen)
            {
                // Buffer is full. Return here and let translayer send
                // the data.

                return eCodes::ST_BufferFull;
            }

            // Fall through, next, encode header.
        }
        // No break;

        case eWriteState::Header:
        {
            writeHeader(true, true);

            _writeState = eWriteState::Binary;
            auto& bin   = _currPkt->getBinary();
            if (bin.empty())
            {
                return eCodes::ST_Complete;
            }

            _srcPtr     = &(*bin.begin());
            _srcEndPtr  = &(*bin.end());
            _maskKeyIdx = 0;
            // Fall through here.
        }
        // No break;

        case eWriteState::Binary:
        {
            // Encode
            _maskBeginPtr = _currPtr;
            for (; _srcPtr != _srcEndPtr && _currPtr != &(*_sendVec.end());
                 ++_srcPtr, ++_currPtr)
            {
                *_currPtr = *_srcPtr;
            }

            if (_needMask)
            {
                maskPacket(_maskBeginPtr, _currPtr);
            }

            if (_srcPtr != _srcEndPtr)
            {
                return eCodes::ST_BufferFull;
            }
        }
        break;

        default:
        {
            PARROT_ASSERT(false);
        }
        break;
    }

    return eCodes::ST_Complete;
}

eCodes WsEncoder::encodeClosePacket()
{
    switch (_writeState)
    {
        case eWriteState::None:
        {
            auto& reason = _currPkt->getCloseReason();
            _payloadLen  = 2 + reason.size();

            if (_payloadLen > WsConfig::_maxPayloadLen)
            {
                LOG_WARN("WsEncoder::encodeClosePacket: Close pkt cannot be "
                         "fragemnted. Reason '"
                         << reason << "' is dropped.");

                // We can check whether _payloadLen == 2 to decide send
                // close reason later or not.
                _payloadLen = 2;
            }
            else if (_payloadLen > 2)
            {
                _srcPtr =
                    reinterpret_cast<const unsigned char*>(&(*reason.begin()));
                _srcEndPtr =
                    reinterpret_cast<const unsigned char*>(&(*reason.end()));
            }

            _writeState = eWriteState::Header;
            if ((_sendVecEndPtr - _currPtr) < _headerLen)
            {
                // Buffer is full. Return here and let translayer send
                // the data.

                return eCodes::ST_BufferFull;
            }

            // Fall through.
        }
        // No break.

        case eWriteState::Header:
        {
            writeHeader(true, true);

            _writeState = eWriteState::Code;
            if ((_sendVecEndPtr - _currPtr) < 2)
            {
                return eCodes::ST_BufferFull;
            }
            _maskKeyIdx = 0;
            // Fall through here.
        }
        // No break;

        case eWriteState::Code:
        {
            _maskBeginPtr = _currPtr;
            *(reinterpret_cast<uint16_t*>(_currPtr)) =
                uniHtons(static_cast<uint16_t>(_currPkt->getCloseCode()));
            _currPtr += 2;

            if (_payloadLen == 2)
            {
                // Do not need to copy reason.
                if (_needMask)
                {
                    maskPacket(_maskBeginPtr, _currPtr);
                }
                return eCodes::ST_Complete;
            }

            if (_currPtr == _sendVecEndPtr)
            {
                // No space left.
                if (_needMask)
                {
                    maskPacket(_maskBeginPtr, _currPtr);
                }
                return eCodes::ST_BufferFull;
            }

            _writeState = eWriteState::Reason;
        }
        // No break;

        case eWriteState::Reason:
        {
            // Encode
            if (!_maskBeginPtr)
            {
                _maskBeginPtr = _currPtr;
            }
            for (; _srcPtr != _srcEndPtr && _currPtr != _sendVecEndPtr;
                 ++_srcPtr, ++_currPtr)
            {
                *_currPtr = *_srcPtr;
            }

            if (_needMask)
            {
                maskPacket(_maskBeginPtr, _currPtr);
            }

            if (_srcPtr != _srcEndPtr)
            {
                return eCodes::ST_BufferFull;
            }
        }
        // No break;

        default:
        {
        }
        break;
    }

    return eCodes::ST_Complete;
}

eCodes WsEncoder::encodeDataPacket()
{
    if (_currPkt->isRaw())
    {
        encodeRawPacket();
    }
    else
    {
        encodePlainPacket();
    }
    return eCodes::ST_Ok;
}

void WsEncoder::computeComponentLen(uint64_t payloadLen)
{
    static_assert(WsConfig::_maxPayloadLen <= 256,
                  "Max payload len is too small.");

    if (WsConfig::_maxPayloadLen >= payloadLen)
    {
        // Payload is less than the max payload len, no fragment.
        _payloadLen = payloadLen;
        _fragmented = false;
    }
    else
    {
        _payloadLen = WsConfig::_maxPayloadLen;
        _fragmented = true;
    }

    if (_needMask)
    {
        // Mask needs 4 bytes.
        if (_payloadLen <= 125)
        {
            _headerLen = 6;
        }
        else if (_payloadLen > 125 && _payloadLen <= 0xFFFF)
        {
            _headerLen = 8;
        }
        else
        {
            _headerLen = 14;
        }
    }
    else
    {
        if (payloadLen <= 125)
        {
            _headerLen = 2;
        }
        else if (payloadLen > 125 && payloadLen <= 0xFFFF)
        {
            _headerLen = 4;
        }
        else
        {
            _headerLen = 10;
        }
    }
}

void WsEncoder::writeHeader(bool firstPkt, bool fin)
{
    if (fin)
    {
        if (firstPkt)
        {
            *_currPtr++ = 0x80 | static_cast<uint8_t>(eOpCode::Binary);
        }
        else
        {
            *_currPtr++ = 0x80 | static_cast<uint8_t>(eOpCode::Continue);
        }
    }
    else
    {
        if (firstPkt)
        {
            *_currPtr++ = static_cast<uint8_t>(eOpCode::Binary);
        }
        else
        {
            *_currPtr++ = static_cast<uint8_t>(eOpCode::Continue);
        }
    }

    uint8_t maskBit = 0x00;
    if (_needMask)
    {
        _maskingKey = _random.random32();
        maskBit     = 0x80;
    }

    if (_payloadLen <= 125)
    {
        *_currPtr++ = maskBit | _payloadLen;
    }
    else if (_payloadLen <= 0xFFFF)
    {
        *_currPtr++ = maskBit | 126;
        auto intBE  = uniHtons(static_cast<uint16_t>(_payloadLen));
        std::copy_n(reinterpret_cast<char*>(&intBE), 2, _currPtr);
        _currPtr += 2;
    }
    else
    {
        *_currPtr++ = maskBit | 127;
        auto intBE  = uniHtonll(_payloadLen);
        std::copy_n(reinterpret_cast<char*>(&intBE), 8, _currPtr);
        _currPtr += 8;
    }

    if (_needMask)
    {
        std::copy_n(reinterpret_cast<char*>(&_maskingKey), 4, _currPtr);
        _currPtr += 4;
    }
}

eCodes WsEncoder::encodeRawPacket()
{
    switch (_writeState)
    {
        case eWriteState::None:
        {
            auto& raw = _currPkt->getPayload();
            PARROT_ASSERT(!raw.empty());
            computeComponentLen(raw.size());

            _writeState  = eWriteState::Header;
            _srcPtr      = &(*raw.begin());
            _srcEndPtr   = &(*raw.end());
            _firstPacket = true;
        }
        // No break;

        case eWriteState::Header:
        {
            if ((_sendVecEndPtr - _currPtr) < _headerLen)
            {
                // Left space is not enough.
                return eCodes::ST_BufferFull;
            }

            bool fin = true;
            if (_fragmented)
            {
                if (static_cast<uint64_t>(_srcEndPtr - _srcPtr) <= _payloadLen)
                {
                    // The last packet. fin should be true.

                    _payloadLen = _srcEndPtr - _srcPtr;
                }
                else
                {
                    fin = false;
                }
            }

            writeHeader(_firstPacket, fin);
            if (_firstPacket)
            {
                _firstPacket = false;
            }

            _writeState        = eWriteState::Raw;
            _maskKeyIdx        = 0;
            _payloadEncodedLen = 0;
        }
        // No break;

        case eWriteState::Raw:
        {
            _maskBeginPtr = _currPtr;
            for (;
                 _payloadEncodedLen < _payloadLen && _currPtr != _sendVecEndPtr;
                 ++_payloadEncodedLen, ++_currPtr)
            {
                *_currPtr = _srcPtr[_payloadEncodedLen];
            }

            if (_needMask)
            {
                maskPacket(_maskBeginPtr, _currPtr);
            }

            if (_payloadEncodedLen == _payloadLen)
            {
                // Encoded one packet.
                _srcPtr += _payloadEncodedLen;
                if (_srcPtr == _srcEndPtr)
                {
                    // WsPacket has been encoded.
                    return eCodes::ST_Complete;
                }

                // The WsPacket has been fragmented, we need to encode
                // another packet, write header next.
                _writeState = eWriteState::Header;
                return encodeRawPacket();
            }
            else
            {
                // Not enough space to encode a packet.
                return eCodes::ST_BufferFull;
            }
        }
        break;

        default:
        {
            PARROT_ASSERT(false);
        }
    }

    // Should never be here.
    PARROT_ASSERT(false);
    return eCodes::ST_Ok;
}

void WsEncoder::computeLength()
{
    _totalLen = 0;

    // System json.
    auto sysJsonPtr = _currPkt->getSysJson();
    if (sysJsonPtr)
    {
        _sysJsonStr = sysJsonPtr->toString();
        _totalLen += getDataLen(_sysJsonStr.size());
    }
    else
    {
        _sysJsonStr.clear();
    }

    // User's json.
    auto jsonPtr = _currPkt->getJson();
    if (jsonPtr)
    {
        _jsonStr = jsonPtr->toString();
        _totalLen += getDataLen(_jsonStr.size());
    }
    else
    {
        _jsonStr.clear();
    }

    // Binary.
    _totalLen += getDataLen(_currPkt->getBinary().size());

    computeComponentLen(_totalLen);
}

eCodes WsEncoder::encodeMeta()
{
    if (!_maskBeginPtr)
    {
        _maskBeginPtr = _currPtr;
    }

    uint8_t copyLen = (_sendVecEndPtr - _currPtr > _srcEndPtr - _currPtr)
                          ? (_srcEndPtr - _currPtr)
                          : (_sendVecEndPtr - _currPtr);

    std::memcpy(_currPtr, _srcPtr, copyLen);
    _payloadEncodedLen += copyLen;
    _encodedLen += copyLen;
    _currPtr += copyLen;
    _srcPtr += copyLen;

    PARROT_ASSERT(_payloadEncodedLen < _payloadLen);

    if (_currPtr == _sendVecEndPtr)
    {
        // Not enough space to encode a packet.
        if (_needMask)
        {
            maskPacket(_maskBeginPtr, _currPtr);
        }
        return eCodes::ST_BufferFull;
    }

    // Meta has been encoded.
    switch (_writeState)
    {
        case eWriteState::SysJsonMeta:
        {
            _writeState = eWriteState::SysJson;
            _srcPtr = reinterpret_cast<unsigned char*>(&(*_sysJsonStr.begin()));
            _srcEndPtr =
                reinterpret_cast<unsigned char*>(&(*_sysJsonStr.end()));
        }
        break;

        case eWriteState::JsonMeta:
        {
            _writeState = eWriteState::Json;
            _srcPtr    = reinterpret_cast<unsigned char*>(&(*_jsonStr.begin()));
            _srcEndPtr = reinterpret_cast<unsigned char*>(&(*_jsonStr.end()));
        }
        break;

        case eWriteState::BinaryMeta:
        {
            auto& bin   = _currPkt->getBinary();
            _writeState = eWriteState::Binary;
            _srcPtr = reinterpret_cast<const unsigned char*>(&(*bin.begin()));
            _srcEndPtr = reinterpret_cast<const unsigned char*>(&(*bin.end()));
        }
        break;

        default:
        {
            PARROT_ASSERT(false);
        }
    }
    return eCodes::ST_Ok;
}

eCodes WsEncoder::encodeData()
{
    if (!_maskBeginPtr)
    {
        _maskBeginPtr = _currPtr;
    }

    uint64_t copyLen = static_cast<uint64_t>(_sendVecEndPtr - _currPtr) >
                               (_payloadLen - _payloadEncodedLen)
                           ? (_payloadLen - _payloadEncodedLen)
                           : (_sendVecEndPtr - _currPtr);

    copyLen = copyLen > static_cast<uint64_t>(_srcEndPtr - _srcPtr)
                  ? (_srcEndPtr - _srcPtr)
                  : copyLen;

    std::memcpy(_currPtr, _srcPtr, copyLen);

    _payloadEncodedLen += copyLen;
    _currPtr += copyLen;
    _srcPtr += copyLen;
    _encodedLen += copyLen;

    if (_encodedLen == _totalLen)
    {
        // Completed, we have encoded all data.
        if (_needMask)
        {
            maskPacket(_maskBeginPtr, _currPtr);
        }
        return eCodes::ST_Complete;
    }

    if (_currPtr == _sendVecEndPtr)
    {
        // Still has data to encode, but buffer is full.
        if (_needMask)
        {
            maskPacket(_maskBeginPtr, _currPtr);
        }
        return eCodes::ST_BufferFull;
    }
    else
    {
        if (_payloadEncodedLen == _payloadLen)
        {
            // Packet is fragmented.
            _prevWriteState = _writeState;
            _writeState     = eWriteState::Header;
            return encodePlainPacket();
        }
        else
        {
            switch (_writeState)
            {
                case eWriteState::SysJson:
                {
                    PARROT_ASSERT(_currPtr == _srcEndPtr);
                    // SysJson is encoded.
                    if (!_jsonStr.empty())
                    {
                        getMetaData(ePayloadItem::Json, _sysJsonStr.size());
                        _writeState = eWriteState::JsonMeta;
                    }
                    else if (!_currPkt->getBinary().empty())
                    {
                        getMetaData(ePayloadItem::Binary,
                                    _currPkt->getBinary().size());
                        _writeState = eWriteState::BinaryMeta;
                    }
                    else
                    {
                        PARROT_ASSERT(false);
                    }

                    _srcPtr    = &(*_metaData.begin());
                    _srcEndPtr = &(*_metaData.end());
                }
                break;

                case eWriteState::Json:
                {
                    PARROT_ASSERT(_currPtr == _srcEndPtr);
                    auto& bin = _currPkt->getBinary();
                    PARROT_ASSERT(!bin.empty());

                    getMetaData(ePayloadItem::Binary, bin.size());
                    _writeState = eWriteState::BinaryMeta;
                    _srcPtr     = &(*_metaData.begin());
                    _srcEndPtr  = &(*_metaData.end());
                }
                break;

                case eWriteState::Binary:
                {
                }
                break;

                default:
                {
                    PARROT_ASSERT(false);
                }
            }
        }
    }

    return eCodes::ST_Ok;
}

eCodes WsEncoder::encodePlainPacket()
{
    eCodes code = eCodes::ST_Ok;

    switch (_writeState)
    {
        case eWriteState::None:
        {
            computeLength();
            _writeState  = eWriteState::Header;
            _firstPacket = true;
            _encodedLen  = 0;
        }
        // No break;

        case eWriteState::Header:
        {
            if ((_sendVecEndPtr - _currPtr) < _headerLen)
            {
                // Left space is not enough.
                return eCodes::ST_BufferFull;
            }

            bool fin = true;
            if (_fragmented)
            {
                if (_totalLen - _encodedLen <= _payloadLen)
                {
                    // The last packet. fin should be true.
                    _payloadLen = _totalLen - _encodedLen;
                }
                else
                {
                    fin = false;
                }
            }

            writeHeader(_firstPacket, fin);

            // Reset mask function related variables.
            _maskKeyIdx   = 0;
            _maskBeginPtr = _currPtr;

            if (_firstPacket)
            {
                _firstPacket = false;
                getMetaData(ePayloadItem::SysJson, _sysJsonStr.size());
                _writeState = eWriteState::SysJsonMeta;
                _srcPtr     = &(*_metaData.begin());
                _srcEndPtr  = &(*_metaData.end());
            }
            else
            {
                _writeState = _prevWriteState;
                return encodePlainPacket();
            }
        }
        // No break;

        case eWriteState::SysJsonMeta:
        {
            code = encodeMeta();
            if (code == eCodes::ST_BufferFull)
            {
                return code;
            }
        }
        // no break;

        case eWriteState::SysJson:
        {
            code = encodeData();
            if (code == eCodes::ST_BufferFull || code == eCodes::ST_Complete)
            {
                return code;
            }
        }
        // no break;

        case eWriteState::JsonMeta:
        {
            code = encodeMeta();
            if (code == eCodes::ST_BufferFull)
            {
                return code;
            }
        }
        // no break;

        case eWriteState::Json:
        {
            code = encodeData();
            if (code == eCodes::ST_BufferFull || code == eCodes::ST_Complete)
            {
                return code;
            }
        }
        // no break;

        case eWriteState::BinaryMeta:
        {
            code = encodeMeta();
            if (code == eCodes::ST_BufferFull)
            {
                return code;
            }
        }
        // no break;

        case eWriteState::Binary:
        {
            code = encodeData();
            if (code == eCodes::ST_BufferFull || code == eCodes::ST_Complete)
            {
                return code;
            }
        }
        break;

        default:
        {
            PARROT_ASSERT(false);
        }
    }

    return code;
}

void WsEncoder::maskPacket(unsigned char* begin, unsigned char* end)
{
    unsigned char* mp = reinterpret_cast<unsigned char*>(&_maskingKey);

    for (auto it = begin; it != end; ++it)
    {
        *it ^= mp[_maskKeyIdx++ % 4];
    }
}

void WsEncoder::getMetaData(ePayloadItem item, uint64_t dataLen)
{
    _metaData.clear();
    // No data no meta.
    if (dataLen == 0)
    {
        return;
    }

    // First byte is the item type.
    _metaData.push_back(static_cast<unsigned char>(item));

    if (dataLen < 254)
    {
        _metaData.push_back(static_cast<unsigned char>(dataLen));
    }
    else if (dataLen >= 254 && dataLen < 65536)
    {
        _metaData.push_back(254);
        uint16_t len = uniHtons(dataLen);
        std::copy_n(reinterpret_cast<unsigned char*>(&len), 2,
                    std::back_inserter(_metaData));
    }
    else
    {
        _metaData.push_back(255);
        dataLen = uniHtonll(dataLen);
        std::copy_n(reinterpret_cast<unsigned char*>(&dataLen), 8,
                    std::back_inserter(_metaData));
    }
}

uint64_t WsEncoder::getDataLen(uint64_t len)
{
    if (len == 0)
    {
        return 0;
    }
    else if (len < 254)
    {
        // 1 byte type, 1 byte length, and the length of json.
        return (1 + 1 + len);
    }
    else if (len >= 254 && len <= 0xFFFF)
    {
        // 1 byte type, 1 byte length hint, 2 bytes length, and the length
        // of data.
        return (1 + 1 + 2 + len);
    }
    else
    {
        // 1 byte type, 1 byte length hint, 8 bytes length, and the length
        // of data.
        return (1 + 1 + 8 + len);
    }
}
}
