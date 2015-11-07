#ifndef __COMPONENT_WEBSOCKET_INC_WSENCODER_H__
#define __COMPONENT_WEBSOCKET_INC_WSENCODER_H__

#include <vector>
#include <array>
#include <cstdint>

#include "wsDefinition.h"
#include "codes.h"

namespace parrot
{
struct WsConfig;
class WsPacket;
class MtRandom;
class wsTranslayer;

class WsEncoder
{
    enum eEncoderState
    {
        Idle,
        Encoding
    };

  public:
    explicit WsEncoder(WsTranslayer& trans);

  public:
    eCodes loadBuff();

  private:
    void encode();    
    void encodeControlPacket();
    void encodeClosePacket();
    void encodeDataPacket();
    
    uint8_t getHeaderLen(uint64_t payloadLen);
    uint8_t getRouteLen(uint64_t route);
    uint64_t getDataLen(uint64_t len);
    void maskPacket(std::vector<char>::iterator begin,
                    std::vector<char>::iterator end);

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
    eEncoderState _state;
    std::vector<char> &_sendVec;
    uint32_t &_needSendLen;
    std::list<std::unique_ptr<WsPacket>> &_pktList;;
    std::unique_ptr<WsPacket> _currPkt;
    std::vector<char>::iterator _lastIt;
    const WsConfig& _config;
    bool _needMask;
    MtRandom& _random;
    std::array<char, 4> _maskingKey;
    std::vector<char> _jsonMeta;
    std::vector<char> _binaryMeta;
};
}

#endif
