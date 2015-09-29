#include "wsPacket.h"
#include "wsEncoder.h"
#include "wsDefinition.h"
#include "wsConfig.h"
#include "sysHelper.h"

namespace parrot
{
WsEncoder::WsEncoder(std::vector<char>& sendVec,
                     std::vector<char>& fragmentedSendVec,
                     const WsConfig& cfg,
                     MtRandom &r,
                     bool needMask)
    : _sendVec(sendVec),
      _fragmentedSendVec(fragmentedVed),
      _config(cfg),
      _needMask(needMask),
      _random(r),
      _maskingKey(),
      _jsonMeta(9),
      _binaryMeta(9)
{
    _jsonMeta.resize(0);
    _binaryMeta.resize(0);
}

void WsEncoder::encode(const WsPacket& pkt)
{
    auto opCode = pkt.getOpCode();
    if (opCode == eOpCode::Binary)
    {
        encodeDataPacket(pkt);
    }
    else if (opCode == eOpCode::Close)
    {
        encodeClosePacket(pkt);
    }
    else
    {
        encodeControlPacket(pkt);
    }
}

void WsEncoder::encodeControlPacket(const WsPacket& pkt)
{
    auto opCode = pkt.getOpCode();
    auto it = _sendVec.begin();
    *it++ = static_cast<char>(0x80 | opCode);
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

    _sendVec.resize(2);
}

void WsEncoder::encodeClosePacket(const WsPacket& pkt)
{
    auto& reason = pkt.getCloseReason();
    uint32_t payloadLen = 2 + reason.size();
    if (payloadLen > _config._fragmentThreshold)
    {
        LOG_WARN("WsEncoder::encodeClosePacket: Close pkt cannot be "
                 "fragemnted. Reason '"
                 << reason << "' is dropped.");
        reason.clear();
        payloadLen = 2;
    }

    auto it = _sendVec.begin();
    uint8_t maskingFlag = _needMask ? 0x80 | 0x00;
    *it++ = static_cast<char>(0x80 | opCode);

    if (payloadLen < 126)
    {
        *it++ = static_cast<char>(maskingFlag | payloadLen);
    }
    else if (payloadLen >= 126 && payloadLen < 65536)
    {
        *it++ = static_cast<char>(maskingFlag | 126);
        *(static_cast<uint16_t*>(&(*it))) = uniHtons(payloadLen);
        it += 2;
    }
    else
    {
        *it++ = static_cast<char>(maskingFlag | 127);
        *(static_cast<uint64_t*>(&(*it))) = uniHtonll(payloadLen);
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
    *(static_cast<uint16_t*>(&(*it))) =
        uniHtons(static_cast<uint16_t>(pkt.getCloseCode()));
    it += 2;

    std::copy_n(reason.begin(), reason.size(), it);
    if (_needMask)
    {
        maskPacket(rit, it);
    }
    _sendVec.resize(it - _sendVec.begin());
}

void WsEncoder::encodeDataPacket(const WsPacket& pkt)
{
    if (pkt.isPacketUndecoded())
    {
        auto& raw = pkt.getRawData();
        if (raw.size() > _sendVec.capacity())
        {
           _fragmentedSendVec.reserve(raw.size());
            std::copy(raw.begin(), raw.end(),
                      std::back_inserter(_fragmentedSendVec));
        }
        else
        {
            std::copy(raw.begin(), raw.end(), std::back_inserter(_sendVec));
        }
        return;
    }

    uint64_t route = pkt.getRoute();
    auto jsonStr = std::move(pkt.getJson().toString());
    auto binData = pkt.getBinary();
    uint64_t payloadLen = getRouteLen() + getDataLen(jsonStr.size()) +
        getDataLen(binData.size());
    std::vector<char>* vecPtr = nullptr;

    if (headerLen + payload > _sendVec.capacity())
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
                *it++ = static_cast<char>(0x80 | eOpCode::Continue);
            }
            else
            {
                *it++ = static_cast<char>(0x80 | eOpCode::Binary);
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
            *(static_cast<uint16_t*>(&(*it))) = uniHtons(dataFrameLen);
            it += 2;
        }
        else
        {
            *it++ = static_cast<char>(maskingFlag | (uint8_t)127);
            *(static_cast<uint64_t*>(&(*it))) = uniHtons(dataFrameLen);
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
    copyLen = frameLeft > ((srcEnd) - (srcBegin)) ? ((srcEnd) - (srcBegin))    \
                                                  : frameLeft;                 \
    std::copy_n(srcBegin, copyLen, dst);                                       \
    srcBegin += copyLen;                                                       \
    buffLeft -= copyLen;                                                       \
    dst += copyLen;                                                            \
    if (buffLeft == 0)                                                         \
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
            _lastIt = _binaryMeta.begin();
        }

        // 4th, write json.
        if (!json.empty())
        {
            COPY_DATA(lastIt, _jsonMeta.end(), it)
            json.clear();
            _lastIt = _binaryMeta.begin();
        }

        // 5th, write binary meta.
        if (!_binaryMeta.empty())
        {
            COPY_DATA(lastIt, _binaryMeta.end(), it)
            _binaryMeta.clear();
            _lastIt = binData.begin();
        }

        // 6th, write binary.
        if (!binData.empty())
        {
            COPY_DATA(lastIt, binData.end(), it)
            _lastIt = binData.begin();
        }
    }

    vecPtr->resize(it - vecPtr.begin());
}

void WsEncoder::maskPacket(std::vector<char>::iterator begin,
                           std::vector<char>::iterator end)
{
    uint8_t i = 0;
    std::for_each(begin, end, [&i](char& c)
                  {
                      c ^= _maskingKey[i++];
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

uint8_t WsEncoder::getRouteLen()
{
    if (_route < 254)
    {
        return (uint8_t)1;
    }
    else if (_route >= 254 && _route < 65536)
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
