#ifndef __COMPONENT_WEBSOCKET_INC_WSENCODER_H__
#define __COMPONENT_WEBSOCKET_INC_WSENCODER_H__

#include <vector>
#include <array>
#include <list>
#include <cstdint>

#include "wsDefinition.h"
#include "codes.h"

namespace parrot
{
struct WsConfig;
class WsPacket;
class MtRandom;
class WsTranslayer;

// WsEncoder is the helper class of WsTranslayer. It encodes the WsPacket
// into raw buff. If the packet is too big, it will be fragmented.
class WsEncoder
{
    enum class eEncoderState
    {
        Idle,
        Encoding
    };

    enum class eWriteState
    {
        None,
        Route,
        Json,
        Binary
    };

  public:
    explicit WsEncoder(WsTranslayer& trans);

  public:
    // This function will get the packet from WsTranslayer's packet, and
    // encodes the packet into buffer.
    //
    // @return:
    //  * ST_Ok         Successfully loaded buffer.
    //  * ST_Complete   No packet to encode.
    eCodes loadBuff();

  private:
    void encode();
    void encodeControlPacket();
    void encodeClosePacket();
    void encodeDataPacket();
    void encodeRawPacket();
    void encodePlainPacket();

    // Compute header length, payload lenght etc. If need to mask packet,
    // call this function.
    void computeLengthNeedMask(uint64_t pktLen);

    // Compute header length, payload length etc. If don't need to mask packet,
    // call this function.
    void computeLengthNoMask(uint64_t pktLen);

    // Compute total length, header length and payload length based on mask
    // flag.
    void computeLength();

    void writeHeader(bool firstPkt, bool fin);
    void writeRoute();
    eCodes writeBuff(const char* src, uint64_t len);

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
    void getMetaData(ePayloadItem item, uint64_t dataLen);

  private:
    // The state of this encoder.
    eEncoderState _state;

    // This member is useful for plain packet. The main purpose of this member
    // is to help encoding big packet which needs to be fragmented.
    eWriteState _writeState;

    // The length of header.
    uint8_t _headerLen;

    // The payload length.
    uint64_t _payloadLen;

    // Need to encoding meta data?
    bool _encodingMeta;

    // This member is used to record how many bytes the item (meta/json/bin)
    // has been encoded.
    uint64_t _itemEncodedLen;

    // The member is used to record how many bytes the packet has been
    // encoded.
    uint64_t _encodedLen;

    // The total length of the packet.
    uint64_t _totalLen;

    // The packet is fragmented?
    bool _fragmented;

    std::vector<char>& _sendVec;
    uint32_t& _needSendLen;
    std::list<std::unique_ptr<WsPacket>>& _pktList;

    // The current encoding packet.
    std::unique_ptr<WsPacket> _currPkt;
    std::vector<char>::iterator _lastIt;
    const WsConfig& _config;

    // The packet needs to be masked?
    bool _needMask;
    MtRandom& _random;

    // Save json string.
    std::string _jsonStr;
    uint32_t _maskingKey;
    std::vector<char> _metaData;
};
}

#endif
