#include "wsEncoder.h"

namespace parrot
{
WsEncoder::WsEncoder(std::vector<char>& sendVec,
                     std::vector<char>& fragmentedSendVec, const WsConfig& cfg,
                     bool needMask)
    : _sendVec(sendVec),
      _fragmentedSendVec(fragmentedVed),
      _config(cfg),
      _needMask(needMask)
{
}

void WsEncoder::toWsDataFrame(const WsPacket& pkt)
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

    std::string jsonStr;
    uint32_t binLen = 0;
    uint32_t pktLen = 0;

    if (_jsonData)
    {
        jsonStr = std::move(_jsonData->toString());
    }

    if (_binData)
    {
        binLen = _binData->size();
    }

    pktLen = getPacketLen(_jsonStr.size(), binLen);

    if (pktLen > _sendVec.capacity())
    {
        writeToBuffer(_fragmentedSendVec, pktLen);
    }
    else
    {
        writeToBuffer(_sendVec, pktLen);
    }
}

void WsEncoder::writeToBuffer(std::vector<char>& buffVec, uint32_t pktLen)
{

    for (uint32_t i = 0; i < pktLen; i)
    {
    }
}

uint32_t WsPacket::getPacketLen(uint32_t jsonLen, uint32_t binLen)
{
    uint32_t pktLen = getRouteLen();
    pktLen += getDataLen(jsonLen);
    pktLen += getDataLen(binLen);

    // If we implement a websocket client, we need to add 4 bytes as
    // mask key.

    if (pktLen < 126)
    {
        pktLen += 2; // 2 bytes header.
    }
    else if (pktLen >= 126 && pktLen < 65536)
    {
        pktLen += (2 + 2); // 4 bytes header includes 2 bytes length.
    }
    else
    {
        pktLen += (2 + 8); // 10 bytes header includes 8 bytes length.
    }

    return pktLen;
}

uint8_t WsPacket::getRouteLen()
{
    if (_route < 254)
    {
        return 1;
    }
    else if (_route >= 254 && _route < 65536)
    {
        return 3; // 1 + 2 bytes.
    }
    else
    {
        return 5; // 1 + 4 bytes.
    }
}

uint32_t WsPacket::getDataLen(uint32_t len)
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
