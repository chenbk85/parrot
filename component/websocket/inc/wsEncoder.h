#ifndef __COMPONENT_WEBSOCKET_INC_WSENCODER_H__
#define __COMPONENT_WEBSOCKET_INC_WSENCODER_H__

#include <array>
#include <cstdint>
#include <list>
#include <vector>

#include "codes.h"
#include "wsDefinition.h"

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
        Header,
        SysJsonMeta,
        SysJson,
        JsonMeta,
        Json,
        BinaryMeta,
        Binary,
        Raw,
        Code,  // for close packet.
        Reason // for close packet.
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
    eCodes encode();
    eCodes encodePingPong();
    eCodes encodeClosePacket();
    eCodes encodeDataPacket();
    eCodes encodeRawPacket();
    eCodes encodePlainPacket();

    /**
     * The function computes the _headerLen and the _payloadLen by the
     * total payloadLen. If payloadLen is less than the max payload len
     * specified in WsConfig, _payloadLen will be equal to the payloadLen.
     * Or, the packet will be fragmented, the _payloadLen will be equal to
     * the max payload len defined in WsConfig.
     *
     * @param  payloadLen         The total length of payload.
     */
    void computeComponentLen(uint64_t payloadLen);

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
    eCodes writeBuff(const unsigned char* src, uint64_t len);
    eCodes writePacketItem(ePayloadItem         item,
                           const unsigned char* buff,
                           uint64_t             buffSize);

    uint64_t getDataLen(uint64_t len);
    void maskPacket(unsigned char* begin, unsigned char* end);

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
    // encoded. When the packet is encoded, the _encodedLen should be
    // equal to _totalLen.
    uint64_t _encodedLen;

    // The total length of the packet payload. If packet is fragmented,
    // _totalLen is the sum of all fragmented packet's payload.
    uint64_t _totalLen;

    // The packet is fragmented?
    bool _fragmented;

    std::vector<unsigned char>&           _sendVec;
    uint32_t&                             _needSendLen;
    std::list<std::unique_ptr<WsPacket>>& _pktList;

    // The current encoding packet.
    std::unique_ptr<WsPacket>            _currPkt;
    std::vector<unsigned char>::iterator _lastIt;
    const WsConfig&                      _config;

    // The packet needs to be masked?
    bool      _needMask;
    MtRandom& _random;

    // Save sys json string.
    std::string _sysJsonStr;
    // Save json string.
    std::string                _jsonStr;
    uint32_t                   _maskingKey;
    uint8_t                    _maskKeyIdx;
    std::vector<unsigned char> _metaData;

    std::vector<unsigned char> _headerVec;
    unsigned char*             _currPtr;
    const unsigned char* const _sendVecEndPtr;
    const unsigned char*       _srcPtr;
    const unsigned char*       _srcEndPtr;
};
}

#endif
