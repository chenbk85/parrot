#include <algorithm>
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
    : _state(eEncoderState::Idle),
      _writeState(eWriteState::None),
      _headerLen(0),
      _payloadLen(0),
      _encodingMeta(false),
      _itemEncodedLen(0),
      _encodedLen(0),
      _fragmented(false),
      _sendVec(trans._sendVec),
      _needSendLen(trans._needSendLen),
      _pktList(trans._pktList),
      _currPkt(),
      _lastIt(),
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
      _sendVecEndPtr(&(*_sendVec.begin()) + _sendVec.capacity()),
      _srcPtr(nullptr),
      _srcEndPtr(nullptr)
{
    // Clear default value.
    _metaData.clear();
}

// eCodes WsEncoder::loadBuff()
// {
//     // If the packet is control packet (include close packet), according
//     // to the RFC6455, it MUST not be fragmented. So call loadBuff one
//     // time will definitely encode the packet to buffer. If the packet is
//     // data, then it can be fragmented. If the packet needs to be fragmented,
//     // the uplayer should call loadBuff multi times to encode the packet
//     // to buffer completely.

//     if (_state == eEncoderState::Idle)
//     {
//         if (_pktList.empty())
//         {
//             return eCodes::ST_Complete;
//         }

//         _currPkt = std::move(*_pktList.begin());
//         _pktList.pop_front();
//     }

//     encode();
//     return eCodes::ST_Ok;
// }

eCodes WsEncoder::loadBuff()
{
    eCodes res;

    while (!_pktList.empty())
    {
        if (_state == eEncoderState::Idle)
        {
            _currPkt = std::move(*_pktList.begin());
            _pktList.pop_front();
        }

        res = encode();

        if (res == eCodes::ST_BufferFull)
        {
            _needSendLen = _currPtr - &(*_sendVec.begin());
            _currPtr     = &(*_sendVec.begin());
            return eCodes::ST_Ok;
        }
        else if (res == eCodes::ST_Complete)
        {
            _state      = eEncoderState::Idle;
            _writeState = eWriteState::None;
            _srcPtr     = nullptr;
            _srcEndPtr  = nullptr;
        }
        else
        {
            PARROT_ASSERT(false);
        }
    }

    return eCodes::ST_Complete;
}

eCodes WsEncoder::encode()
{
    PARROT_ASSERT(_currPkt.get());

    eCodes res;
    auto   opCode = _currPkt->getOpCode();

    if (opCode == eOpCode::Binary)
    {
        res = encodeDataPacket();
    }
    else if (opCode == eOpCode::Close)
    {
        res = encodeClosePacket();
    }
    else if (opCode == eOpCode::Ping || opCode == eOpCode::Pong)
    {
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
            _state          = eEncoderState::Encoding;
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

        } // eWriteState::Header:
        // No break;

        case eWriteState::Binary:
        {
            // Encode
            auto startPtr = _currPtr;
            for (; _srcPtr != _srcEndPtr && _currPtr != &(*_sendVec.end());
                 ++_srcPtr, ++_currPtr)
            {
                *_currPtr = *_srcPtr;
            }

            if (_needMask)
            {
                maskPacket(startPtr, _currPtr);
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
        break;

        case eWriteState::Code:
        {
            auto startPtr = _currPtr;
            *(reinterpret_cast<uint16_t*>(_currPtr)) =
                uniHtons(static_cast<uint16_t>(_currPkt->getCloseCode()));
            _currPtr += 2;

            if (_payloadLen == 2)
            {
                // Do not need to copy reason.
                if (_needMask)
                {
                    maskPacket(startPtr, _currPtr);
                }
                return eCodes::ST_Complete;
            }

            if (_currPtr == _sendVecEndPtr)
            {
                // No space left.

                if (_needMask)
                {
                    maskPacket(startPtr, _currPtr);
                }
                return eCodes::ST_BufferFull;
            }

            _writeState = eWriteState::Reason;
        }
        // No break;

        case eWriteState::Reason:
        {
            // Encode
            auto startPtr = _currPtr;
            for (; _srcPtr != _srcEndPtr && _currPtr != _sendVecEndPtr;
                 ++_srcPtr, ++_currPtr)
            {
                *_currPtr = *_srcPtr;
            }

            if (_needMask)
            {
                maskPacket(startPtr, _currPtr);
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

            _writeState = eWriteState::Header;
            _srcPtr     = &(*raw.begin());
            _srcEndPtr  = &(*raw.end());
        }
        // No break;

        case eWriteState::Header:
        {
            if ((_sendVecEndPtr - _currPtr) < _headerLen)
            {
                return eCodes::ST_BufferFull;
            }

            bool fin = true;
            if (_fragmented)
            {
                if (static_cast<uint64_t>(_srcEndPtr - _srcPtr) <= _payloadLen)
                {
                    _payloadLen = _srcEndPtr - _srcPtr;
                }
                else
                {
                    fin = false;
                }
            }

            writeHeader(_srcPtr == &(_currPkt->getPayload()[0]), fin);
            _writeState = eWriteState::Raw;
            _maskKeyIdx = 0;
            _encodedLen = 0;
        }
        // No break;

        case eWriteState::Raw:
        {
            auto startPtr = _currPtr; // Save the pos for masking.
            for (; _encodedLen < _payloadLen && _currPtr != _sendVecEndPtr;
                 ++_encodedLen, ++_currPtr)
            {
                *_currPtr = _srcPtr[_encodedLen];
            }

            if (_needMask)
            {
                maskPacket(startPtr, _currPtr);
            }

            if (_encodedLen == _payloadLen)
            {
                // Encoded one packet.
                _srcPtr += _encodedLen;
                if (_srcPtr == _srcEndPtr)
                {
                    // WsPacket has been encoded.
                    return eCodes::ST_Complete;
                }

                // The WsPacket has been fragmented, we need to encode the
                // next part of the WsPacket.
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

    return eCodes::ST_Ok;
}

eCodes WsEncoder::writeBuff(const unsigned char* src, uint64_t len)
{
    uint64_t leftLen = _headerLen + _payloadLen - (_lastIt - _sendVec.begin());
    uint64_t needCopyLen = len - _itemEncodedLen;
    uint64_t copyLen     = leftLen >= needCopyLen ? needCopyLen : leftLen;

    std::copy_n(src + _itemEncodedLen, copyLen, _lastIt);

    _itemEncodedLen += copyLen;
    _lastIt += copyLen;
    _encodedLen += copyLen;

    if (_itemEncodedLen == len)
    {
        return eCodes::ST_Complete;
    }

    if (_needMask)
    {
        maskPacket(_sendVec.begin() + _headerLen,
                   _sendVec.begin() + _headerLen + _payloadLen);
    }

    _needSendLen = _lastIt - _sendVec.begin();
    return eCodes::ST_RetryLater;
}

eCodes WsEncoder::writePacketItem(ePayloadItem         item,
                                  const unsigned char* buff,
                                  uint64_t             buffSize)
{
    if (_encodingMeta)
    {
        if (_metaData.empty())
        {
            getMetaData(item, buffSize);
            _itemEncodedLen = 0;
        }

        if (writeBuff(&_metaData[0], _metaData.size()) != eCodes::ST_Complete)
        {
            // Buffer is full. Need to send the buffer then
            // try again.
            return eCodes::ST_RetryLater;
        }
        else
        {
            _encodingMeta   = false;
            _itemEncodedLen = 0;
        }
    }

    if (writeBuff(buff, buffSize) != eCodes::ST_Complete)
    {
        // Buffer is full. Need to send the buffer then try again.
        return eCodes::ST_RetryLater;
    }
    else
    {
        _encodingMeta = true;
    }

    return eCodes::ST_Complete;
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

    if (_needMask)
    {
        computeLengthNeedMask(_totalLen);
    }
    else
    {
        computeLengthNoMask(_totalLen);
    }

    _fragmented = _totalLen > _payloadLen ? true : false;
}

void WsEncoder::encodePlainPacket()
{
    bool finFlag = true;
    if (_state == eEncoderState::Idle)
    {
        _state          = eEncoderState::Encoding;
        _writeState     = eWriteState::SysJson;
        _encodedLen     = 0;
        _itemEncodedLen = 0;
        _encodingMeta   = true;
        _metaData.clear();

        computeLength();

        finFlag = (_totalLen > _payloadLen) ? false : true;
    }
    else
    {
        auto leftLen = _totalLen - _encodedLen;
        finFlag      = (leftLen > _payloadLen) ? false : true;
        if (finFlag)
        {
            // Last packet, we need to recompute the header.
            if (_needMask)
            {
                computeLengthNeedMask(leftLen);
            }
            else
            {
                computeLengthNoMask(leftLen);
            }
        }
    }

    // Write header.
    writeHeader(_encodedLen == 0, finFlag);

    switch (_writeState)
    {
        case eWriteState::SysJson:
        {
            if (!_sysJsonStr.empty())
            {
                if (writePacketItem(
                        ePayloadItem::SysJson,
                        reinterpret_cast<unsigned char*>(&_sysJsonStr[0]),
                        _sysJsonStr.size()) != eCodes::ST_Complete)
                {
                    return;
                }

                _writeState   = eWriteState::Json;
                _encodingMeta = true;
                _metaData.clear();
            }
        }
        // No break;

        case eWriteState::Json:
        {
            if (!_jsonStr.empty())
            {
                if (writePacketItem(
                        ePayloadItem::Json,
                        reinterpret_cast<unsigned char*>(&_jsonStr[0]),
                        _jsonStr.size()) != eCodes::ST_Complete)
                {
                    return;
                }

                _writeState   = eWriteState::Binary;
                _encodingMeta = true;
                _metaData.clear();
            }
        }
        // No break;

        case eWriteState::Binary:
        {
            auto& bin = _currPkt->getBinary();
            if (!bin.empty())
            {
                if (writePacketItem(ePayloadItem::Binary, &bin[0],
                                    bin.size()) != eCodes::ST_Complete)
                {
                    return;
                }

                // _writeState = eWriteState::None;
                // _encodingMeta = true;
                // _metaData.clear();
            }
        }
        break;

        default:
        {
            PARROT_ASSERT(false);
        }
        break;
    }

    _needSendLen = _lastIt - _sendVec.begin();
    _state       = eEncoderState::Idle;
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
    else if (len >= 254 && len < 65536)
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
