#include <string>
#include <algorithm>

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
WsEncoder::WsEncoder(WsTranslayer &trans)
    : _state(eEncoderState::Idle),
      _sendVec(trans._sendVec),
      _needSendLen(trans._needSendLen),
      _pktList(trans._pktList),
      _currPkt(),
      _lastIt(),
      _config(trans._config),
      _needMask(trans._needSendMasked),
      _random(*(trans._random)),
      _maskingKey(),
      _jsonMeta(9),
      _binaryMeta(9)
{
    _jsonMeta.resize(0);
    _binaryMeta.resize(0);
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
    return eCodes::ST_Ok;
}

void WsEncoder::encode()
{
    PARROT_ASSERT(_currPkt.get());
    
    auto opCode = _currPkt.getOpCode();
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
    auto it = _sendVec.begin();
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
    const std::string & reason = pkt.getCloseReason();
    uint32_t payloadLen = 2 + reason.size();
    bool copyReason = true;
    if (payloadLen > _config._fragmentThreshold)
    {
        LOG_WARN("WsEncoder::encodeClosePacket: Close pkt cannot be "
                 "fragemnted. Reason '"
                 << reason <<
                 "' is dropped.");
        payloadLen = 2;
        copyReason = false;
    }

    auto it = _sendVec.begin();
    uint8_t maskingFlag = _needMask ? 0x80 : 0x00;
    *it++ = static_cast<char>(0x80 | (uint8_t)eOpCode::Close);

    if (payloadLen < 126)
    {
        *it++ = static_cast<char>(maskingFlag | payloadLen);
    }
    else if (payloadLen >= 126 && payloadLen < 65536)
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
        for (int i = 0; i != 4; ++i)
        {
            _maskingKey[i] = static_cast<char>(_random.random(256));
            *it++ = _maskingKey[i];
        }
    }

    auto rit = it;
    *(reinterpret_cast<uint16_t*>(&(*it))) =
        uniHtons(static_cast<uint16_t>(pkt.getCloseCode()));
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

void WsEncoder::encodeRawPacket()
{
    auto& raw = _currPkt->getPayload();
    if (raw.size() > _sendVec.capacity())
    {
        if (_state == eEncoderState::Idle)
        {
            _lastIt = raw.begin();
            _state = eEncoderState::Encoding;
        }

        uint32_t copyLen = 0;
        if (raw.end() - _lastIt > _sendVec.capacity())
        {
            copyLen = _sendVec.capacity();
        }
        else
        {
            // Last one.
            copyLen = raw.end() - _lastIt;
            _state = eEncoderState::Idle;
        }

        std::copy_n(_lastIt, copyLen, _sendVec.begin());
        lastIt += copyLen;
        _needSendLen = copyLen;
    }
    else
    {
        std::copy(raw.begin(), raw.end(), _sendVec.begin());
        _needSendLen = raw.size();
    }
    
    return;
}

void WsEncoder::encodePlainPacket()
{
    uint64_t route = pkt.getRoute();
    auto jsonStr = std::move(pkt.getJson().toString());
    auto binData = pkt.getBinary();
    uint64_t payloadLen = getRouteLen(route) + getDataLen(jsonStr.size()) +
        getDataLen(binData.size());
    std::vector<char>* vecPtr = nullptr;

    if (getHeaderLen(payloadLen) + payloadLen > _sendVec.capacity())
    {
        vecPtr = &_fragmentedSendVec;
    }
    else
    {
        vecPtr = &_sendVec;
    }

    // User one loop to write all data to buffer, if we seperate to
    // small functions, we have to define lots of temp vars.
    auto it = vecPtr->begin();
    auto rit = it;
    auto jsonIt = jsonStr.begin();
    auto lastIt = _jsonMeta.begin();
    bool firstDataFrame = true;
    uint64_t dataFrameLen = 0;
    uint64_t frameLeft = 0;
    uint64_t copyLen = 0;
    uint8_t maskingFlag = _needMask ? 0x80 : 0x00;

    // Create meta info.
    getJsonMeta(jsonStr.size());
    getBinaryMeta(binData.size());

    for (uint64_t i = 0; i < payloadLen; i += _config._fragmentThreshold)
    {
        frameLeft = _config._fragmentThreshold;

        // First, write head.
        
        if (i + _config._fragmentThreshold <= payloadLen)
        {
            // Finished.
            if (!firstDataFrame)
            {
                *it++ = static_cast<char>(0x80 | (uint8_t)eOpCode::Continue);
            }
            else
            {
                *it++ = static_cast<char>(0x80 | (uint8_t)eOpCode::Binary);
            }

            // Get the length of payload this data frame.
            dataFrameLen = payloadLen - i;
        }
        else
        {
            // Fragmented.
            if (!firstDataFrame)
            {
                *it++ = static_cast<char>(eOpCode::Continue);
            }
            else
            {
                *it++ = static_cast<char>(eOpCode::Binary);
            }

            // The payload length of this data frame is fragment
            // threshold.
            dataFrameLen = _config._fragmentThreshold;
        }

        if (dataFrameLen < 126)
        {
            *it++ = static_cast<char>(maskingFlag | dataFrameLen);
        }
        else if (payloadLen >= 126 && payloadLen < 65536)
        {
            *it++ = static_cast<char>(maskingFlag | (uint8_t)126);
            *(reinterpret_cast<uint16_t*>(&(*it))) = uniHtons(dataFrameLen);
            it += 2;
        }
        else
        {
            *it++ = static_cast<char>(maskingFlag | (uint8_t)127);
            *(reinterpret_cast<uint64_t*>(&(*it))) = uniHtons(dataFrameLen);
            it += 8;
        }

        if (_needMask)
        {
            for (int i = 0; i != 4; ++i)
            {
                _maskingKey[i] = static_cast<char>(_random.random(256));
                *it++ = _maskingKey[i];
            }
        }

        // Second, write route.

        rit = it; // Remember the position data begins.

        if (firstDataFrame)
        {
            if (route < 254)
            {
                *it++ = static_cast<char>(route);
            }
            else if (route >= 254 && route < 65536)
            {
                *it++ = static_cast<char>(254);
                *(uint16_t*)(&(*it)) = uniHtons(route);
                it += 2;
            }
            else
            {
                *it++ = static_cast<char>(255);
                *(uint64_t*)(&(*it)) = uniHtonll(route);
                it += 8;
            }

            firstDataFrame = false; // Don't need this anymore.
        }

#define COPY_DATA(srcBegin, srcEnd, dst)                                       \
    copyLen = frameLeft > (uint64_t)((srcEnd) - (srcBegin))                    \
                  ? ((srcEnd) - (srcBegin))                                    \
                  : frameLeft;                                                 \
    std::copy_n(srcBegin, copyLen, dst);                                       \
    srcBegin += copyLen;                                                       \
    frameLeft -= copyLen;                                                      \
    dst += copyLen;                                                            \
    if (frameLeft == 0)                                                        \
    {                                                                          \
        if (_needMask)                                                         \
        {                                                                      \
            maskPacket(rit, it);                                               \
            continue;                                                          \
        }                                                                      \
    }

        // Third, write json meta. From here we check the buffer left.
        if (!_jsonMeta.empty())
        {
            COPY_DATA(lastIt, _jsonMeta.end(), it)
            _jsonMeta.clear();
        }
        else
        {
            lastIt = _binaryMeta.begin();
        }

        // 4th, write json.
        if (!jsonStr.empty())
        {
            COPY_DATA(jsonIt, jsonStr.end(), it)
            jsonStr.clear();
            lastIt = _binaryMeta.begin();
        }

        // 5th, write binary meta.
        if (!_binaryMeta.empty())
        {
            COPY_DATA(lastIt, _binaryMeta.end(), it)
            _binaryMeta.clear();
            lastIt = binData.begin();
        }

        // 6th, write binary.
        if (!binData.empty())
        {
            COPY_DATA(lastIt, binData.end(), it)
            lastIt = binData.begin();
        }
    }

    vecPtr->resize(it - vecPtr->begin());
}

void WsEncoder::maskPacket(std::vector<char>::iterator begin,
                           std::vector<char>::iterator end)
{
    uint8_t i = 0;
    std::for_each(begin, end, [&i, this](char& c)
                  {
                      c ^= this->_maskingKey[i++];
                      i %= 4;
                  });
}

void WsEncoder::getJsonMeta(uint64_t dataLen)
{
    getMeta(ePayloadItem::Json, dataLen, _jsonMeta);
}

void WsEncoder::getBinaryMeta(uint64_t dataLen)
{
    getMeta(ePayloadItem::Binary, dataLen, _binaryMeta);
}

void WsEncoder::getMeta(ePayloadItem item,
                        uint64_t dataLen,
                        std::vector<char>& meta)
{
    // No data no meta.
    if (dataLen == 0)
    {
        return;
    }

    auto it = meta.begin();

    // First byte is the item type.
    *it++ = static_cast<char>(item);

    if (dataLen < 254)
    {
        *it++ = static_cast<char>(dataLen);
    }
    else if (dataLen >= 254 && dataLen < 65536)
    {
        *(uint16_t*)(&(*it)) = uniHtons(dataLen);
    }
    else
    {
        *(uint64_t*)(&(*it)) = uniHtonll(dataLen);
    }
}

uint8_t WsEncoder::getHeaderLen(uint64_t payloadLen)
{
    uint8_t headerLen = 0;
    if (payloadLen < 126)
    {
        headerLen = 2;
    }
    else if (payloadLen >= 126 && payloadLen < 65536)
    {
        headerLen = 2 + 2;
    }
    else
    {
        headerLen = 2 + 8;
    }

    if (_needMask)
    {
        headerLen += 4;
    }

    return headerLen;
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
        return (1 + 1 + len); // 1 byte type, 1 byte length
                              // and the length of json.
    }
    else if (len >= 254 && len < 65536)
    {
        return (1 + 3 + len); // 1 byte type, 3 byte length
                              // and the length of json.
    }
    else
    {
        return (1 + 9 + len); // 1 byte type, 9 byte length
                              // and the length of json.
    }
}
}
