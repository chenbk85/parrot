#ifndef __COMPONENT_WEBSOCKET_INC_WSENCODER_H__
#define __COMPONENT_WEBSOCKET_INC_WSENCODER_H__

#include <vector>

namespace parrot
{
class WsConfig;
class WsPacket;
class MtRandom;

class WsEncoder
{
  public:
    WsEncoder(std::vector<char>& sendVec,
              std::vector<char>& fragmentedSendVec,
              const WsConfig& cfg);

  public:
    void encode(const WsPacket& pkt);

  private:
    void encodeControlPacket(const WsPacket& pkt);
    void encodeClosePacket(const WsPacket& pkt);
    void encodeDataPacket(const WsPacket& pkt);
    uint8_t getRouteLen();
    uint64_t getDataLen(uint64_t len);

    // Meta info is for the peer to decode what data is it. The format is
    //            | type field (alwasy 1 byte) | data length field |
    // If data length is less than 254 bytes
    //            | type (1 byte) | data length (1 byte) |
    // If data length is >=254 && data length < 65536
    //            | type (1 byte) | 254 (1 byte) | data length (2 bytes) |
    // else
    //            | type (1 byte) | 255 (1 byte) | data length (8 bytes) |
    void getJsonMeta(uint64_t dataLen);
    void getBinaryMeta(uint64_t dataLen);
    void getMeta(ePayloadItem item, uint64_t dataLen, std::vector<char>& meta);

  private:
    std::vector<char> _sendVec;
    std::vector<char> _fragmentedSendVec;
    const WsConfig& _config;
    bool _needMask;
    MtRandom& _random;
    std::array<char, 4> _maskingKey;
    std::vector<char> _jsonMeta;
    std::vecotr<char> _binaryMeta;
};
}

#endif
