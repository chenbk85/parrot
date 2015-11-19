#include <string>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>

#include "macroFuncs.h"
#include "json.h"
#include "logger.h"
#include "wsPacket.h"
#include "wsEncoder.h"
#include "wsDefinition.h"
#include "wsConfig.h"
#include "sysHelper.h"
#include "mtRandom.h"
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
      _jsonStr(),
      _maskingKey(0),
      _metaData(9)
{
    _metaData.resize(0);

    // Min buffer size should not less than 134.
    PARROT_ASSERT(_sendVec.capacity() >= 134);
}

eCodes WsEncoder::loadBuff()
{
    // If the packet is control packet (include close packet), according
    // to the RFC6455, it should not be fragmented. So call loadBuff one
    // time will definitely encode the packet to buffer. If the packet is
    // data, then it can be fragmented. If the packet needs to be fragmented,
    // the uplayer should call loadBuff multi times to encode the packet
    // to buffer completely.

    if (_state == eEncoderState::Idle)
    {
        if (_pktList.empty())
        {
            return eCodes::ST_Complete;
        }

        _currPkt = std::move(*_pktList.begin());
        _pktList.pop_front();
    }

    encode();

    std::ostringstream ostr;
    ostr << std::showbase << std::internal << std::setfill('0');
    for (uint64_t i = 0; i != _needSendLen; ++i)
    {
        std::cout << static_cast<uint32_t>(_sendVec[i]) << " ";
        ostr << std::hex << std::setw(4) << (uint32_t)_sendVec[i] << std::dec
             << " ";
    }
    std::cout << std::endl;
    LOG_DEBUG("WsEncoder::loadBuff: _needSendLen is "
              << _needSendLen << ". Send buff is " << ostr.str() << ".");
    return eCodes::ST_Ok;
}

void WsEncoder::encode()
{
    PARROT_ASSERT(_currPkt.get());

    auto opCode = _currPkt->getOpCode();
    if (opCode == eOpCode::Binary)
    {
        encodeDataPacket();
    }
    else if (opCode == eOpCode::Close)
    {
        encodeClosePacket();
    }
    else
    {
        encodeControlPacket();
    }
}

void WsEncoder::encodeControlPacket()
{
    auto opCode = _currPkt->getOpCode();
    auto it     = _sendVec.begin();
    *it++ = static_cast<char>(0x80 | (uint8_t)opCode);
    if (_needMask)
    {
        *it++ = static_cast<char>(0x80);
        for (int i = 0; i != 4; ++i)
        {
            *it++ = static_cast<char>(_random.random(256));
        }
    }
    else
    {
        *it++ = static_cast<char>(0x00);
    }

    _needSendLen = 2;
}

void WsEncoder::encodeClosePacket()
{
    const std::string& reason = _currPkt->getCloseReason();
    uint32_t payloadLen       = 2 + reason.size();
    bool copyReason = true;
    if (payloadLen > _config._fragmentThreshold)
    {
        LOG_WARN("WsEncoder::encodeClosePacket: Close pkt cannot be "
                 "fragemnted. Reason '"
                 << reason << "' is dropped.");
        payloadLen = 2;
        copyReason = false;
    }

    auto it             = _sendVec.begin();
    uint8_t maskingFlag = _needMask ? 0x80 : 0x00;
    *it++               = static_cast<char>(0x80 | (uint8_t)eOpCode::Close);

    if (payloadLen < 126)
    {
        *it++ = static_cast<char>(maskingFlag | payloadLen);
    }
    else if (payloadLen >= 126 && payloadLen <= 0xFFFF)
    {
        *it++ = static_cast<char>(maskingFlag | 126);
        *(reinterpret_cast<uint16_t*>(&(*it))) = uniHtons(payloadLen);
        it += 2;
    }
    else
    {
        *it++ = static_cast<char>(maskingFlag | 127);
        *(reinterpret_cast<uint64_t*>(&(*it))) = uniHtonll(payloadLen);
        it += 8;
    }

    if (_needMask)
    {
        _maskingKey = _random.random32();
        // Do not treat _masking key as integer.
        std::copy_n(reinterpret_cast<char*>(&_maskingKey), 4, it);
        it += 4;
    }

    auto rit = it;
    *(reinterpret_cast<uint16_t*>(&(*it))) =
        uniHtons(static_cast<uint16_t>(_currPkt->getCloseCode()));
    it += 2;

    if (copyReason)
    {
        std::copy_n(reason.begin(), reason.size(), it);
        it += reason.size();
    }

    if (_needMask)
    {
        maskPacket(rit, it);
    }
    _needSendLen = it - _sendVec.begin();
}

void WsEncoder::encodeDataPacket()
{
    if (_currPkt->isRaw())
    {
        encodeRawPacket();
    }
    else
    {
        encodePlainPacket();
    }
}

void WsEncoder::computeLengthNeedMask(uint64_t pktLen)
{
    if (pktLen <= 125)
    {
        _headerLen = 6;
    }
    else if (pktLen > 125 && pktLen <= 0xFFFF)
    {
        _headerLen = 8;
    }
    else
    {
        // 65550 (14 + 65536) is the minimal length if header is 10 bytes.
        // If packet length is greater than 2^16 - 1, but send buffer length
        // is less than 65550, we need to fragment.
        if (_sendVec.capacity() < 65550)
        {
            _headerLen = 8;
        }
        else
        {
            _headerLen = 14;
        }
    }

    if (pktLen <= _sendVec.capacity() - _headerLen)
    {
        _payloadLen = pktLen;
    }
    else
    {
        _payloadLen = _sendVec.capacity() - _headerLen;
    }

    if (_headerLen == 4)
    {
        if (_payloadLen > 65535)
        {
            _payloadLen = 65535;
        }
    }
}

void WsEncoder::computeLengthNoMask(uint64_t pktLen)
{
    if (pktLen <= 125)
    {
        _headerLen = 2;
    }
    else if (pktLen > 125 && pktLen <= 0xFFFF)
    {
        _headerLen = 4;
    }
    else
    {
        // 65546 (10 + 65536) is the minimal length if header is 10 bytes.
        // If packet length is greater than 2^16 - 1, but send buffer length
        // is less than 65546, we need to fragment.
        if (_sendVec.capacity() < 65546)
        {
            _headerLen = 4;
        }
        else
        {
            _headerLen = 10;
        }
    }

    if (pktLen <= _sendVec.capacity() - _headerLen)
    {
        // Here, we have enough space to encode this packet. So _payloadlen
        // is equal to the packetLen.
        _payloadLen = pktLen;
    }
    else
    {
        // We don't have enough space to encode the whole packet.
        _payloadLen = _sendVec.capacity() - _headerLen;
    }

    if (_headerLen == 4)
    {
        // if _headerLen is 4 bytes, the length of the payload should less
        // than or equal to 0xFFFF.
        if (_payloadLen > 0xFFFF)
        {
            _payloadLen = 0xFFFF;
        }
    }
}

void WsEncoder::writeHeader(bool firstPkt, bool fin)
{
    _lastIt = _sendVec.begin();
    if (fin)
    {
        if (firstPkt)
        {
            *_lastIt++ = 0x80 | static_cast<uint8_t>(eOpCode::Binary);
        }
        else
        {
            *_lastIt++ = 0x80 | static_cast<uint8_t>(eOpCode::Continue);
        }
    }
    else
    {
        if (firstPkt)
        {
            *_lastIt++ = static_cast<uint8_t>(eOpCode::Binary);
        }
        else
        {
            *_lastIt++ = static_cast<uint8_t>(eOpCode::Continue);
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
        *_lastIt++ = maskBit | _payloadLen;
    }
    else if (_payloadLen <= 0xFFFF)
    {
        *_lastIt++ = maskBit | 126;
        auto intBE = uniHtons(static_cast<uint16_t>(_payloadLen));
        std::copy_n(_lastIt, 2, reinterpret_cast<char*>(&intBE));
        _lastIt += 2;
    }
    else
    {
        *_lastIt   = maskBit | 127;
        auto intBE = uniHtonll(_payloadLen);
        std::copy_n(_lastIt, 8, reinterpret_cast<char*>(&intBE));
        _lastIt += 8;
    }

    if (_needMask)
    {
        std::copy_n(reinterpret_cast<char*>(&_maskingKey), 4, _lastIt);
        _lastIt += 4;
    }
}

void WsEncoder::encodeRawPacket()
{
    auto& raw         = _currPkt->getPayload();
    bool finFlag      = false;
    bool firstPktFlag = true;

    PARROT_ASSERT(!raw.empty());

    if (_state == eEncoderState::Idle)
    {
        _state    = eEncoderState::Encoding;
        _totalLen = raw.size();

        if (_needMask)
        {
            computeLengthNeedMask(_totalLen);
        }
        else
        {
            computeLengthNoMask(_totalLen);
        }

        if (_totalLen > _payloadLen)
        {
            _fragmented = true;
            finFlag     = false;
        }
        else
        {
            _fragmented = false;
            finFlag     = true;
        }
        _encodedLen = 0;
    }
    else
    {
        firstPktFlag = false;
        auto leftLen = _totalLen - _encodedLen;
        if (leftLen <= _payloadLen)
        {
            finFlag = true;

            // Recompute header.
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

    auto wLen = 0u;
    if (!finFlag)
    {
        wLen = _payloadLen;
    }
    else
    {
        wLen = _totalLen - _encodedLen;
    }

    writeHeader(firstPktFlag, finFlag);
    std::copy_n(raw.begin() + _encodedLen, wLen, _lastIt);
    _encodedLen += wLen;
    _lastIt += wLen;
    _needSendLen = _lastIt - _sendVec.begin();

    if (_encodedLen == _totalLen)
    {
        _state = eEncoderState::Idle;
    }
    return;
}

void WsEncoder::writeRoute()
{
    auto route = _currPkt->getRoute();
    if (route < 254)
    {
        *_lastIt++ = static_cast<uint8_t>(route);
        _encodedLen += 1;
    }
    else if (route < 0xFFFF)
    {
        *_lastIt++   = 254;
        uint16_t len = uniHtons(static_cast<uint16_t>(route));
        std::copy_n(&len, 2, _lastIt);
        _encodedLen += 2;
        _lastIt += 2;
    }
    else
    {
        *_lastIt++   = 254;
        uint64_t len = uniHtonll(route);
        std::copy_n(&len, 8, _lastIt);
        _encodedLen += 9;
        _lastIt += 8;
    }
}

eCodes WsEncoder::writeBuff(const unsigned char* src, uint64_t len)
{
    uint64_t leftLen    = _headerLen + _payloadLen -
        (_lastIt - _sendVec.begin());
    uint8_t needCopyLen = len - _itemEncodedLen;
    uint8_t copyLen =
        (leftLen >= needCopyLen) ? needCopyLen : needCopyLen - leftLen;
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

void WsEncoder::computeLength()
{
    _totalLen    = getRouteLen(_currPkt->getRoute());
    auto jsonPtr = _currPkt->getJson();
    if (jsonPtr)
    {
        _jsonStr = std::move(jsonPtr->toString());
        _totalLen += getDataLen(_jsonStr.size());
    }
    else
    {
        _jsonStr.clear();
    }

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
        _writeState     = eWriteState::Route;
        _encodedLen     = 0;
        _itemEncodedLen = 0;

        computeLength();

        finFlag = (_totalLen > _payloadLen) ? false : true;
    }
    else
    {
        auto leftLen = _totalLen - _encodedLen;
        finFlag = (leftLen > _payloadLen) ? false : true;
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
        case eWriteState::Route:
        {
            writeRoute();
            _writeState   = eWriteState::Json;
            _encodingMeta = true;
            _metaData.clear();
        }
        // No break;

        case eWriteState::Json:
        {
            if (!_jsonStr.empty())
            {
                if (_encodingMeta)
                {
                    if (_metaData.empty())
                    {
                        getMetaData(ePayloadItem::Json, _jsonStr.size());
                        _itemEncodedLen = 0;
                    }

                    if (writeBuff(&_metaData[0], _metaData.size()) !=
                        eCodes::ST_Complete)
                    {
                        // Buffer is full. Need to send the buffer then
                        // try again.
                        return;
                    }
                    else
                    {
                        _itemEncodedLen = 0;
                    }
                }

                if (writeBuff(reinterpret_cast<unsigned char*>(&_jsonStr[0]),
                              _jsonStr.size()) != eCodes::ST_Complete)
                {
                    // Buffer is full. Need to send the buffer then try again.
                    return;
                }
                else
                {
                    _encodingMeta = true;
                }
            }
        }
        // No break;

        case eWriteState::Binary:
        {
            auto& bin = _currPkt->getBinary();
            if (!bin.empty())
            {
                if (_encodingMeta)
                {
                    if (_metaData.empty())
                    {
                        getMetaData(ePayloadItem::Json, bin.size());
                        _itemEncodedLen = 0;
                    }

                    if (writeBuff(&_metaData[0], _metaData.size()) !=
                        eCodes::ST_Complete)
                    {
                        // Buffer is full. Need to send the buffer then
                        // try again.
                        return;
                    }
                    else
                    {
                        _itemEncodedLen = 0;
                    }
                }

                if (writeBuff(&bin[0], bin.size()) != eCodes::ST_Complete)
                {
                    // Buffer is full. Need to send the buffer then try again.
                    return;
                }
                else
                {
                    _encodingMeta = true;
                }
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

void WsEncoder::maskPacket(std::vector<unsigned char>::iterator begin,
                           std::vector<unsigned char>::iterator end)
{
    uint8_t i   = 0;
    uint8_t* mp = reinterpret_cast<uint8_t*>(&_maskingKey);

    for (auto it = begin; it != end; ++it)
    {
        *it ^= mp[i++ % 4];
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
        dataLen = uniHtons(dataLen);
        std::copy_n(reinterpret_cast<unsigned char*>(&dataLen), 8,
                    std::back_inserter(_metaData));
    }
}

uint8_t WsEncoder::getRouteLen(uint64_t route)
{
    if (route < 254)
    {
        return (uint8_t)1;
    }
    else if (route >= 254 && route < 65536)
    {
        return (uint8_t)3; // 1 + 2 bytes.
    }
    else
    {
        return (uint8_t)9; // 1 + 8 bytes.
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
