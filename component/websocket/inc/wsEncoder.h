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

/**
 * WsEncoder is the helper class of WsTranslayer. It encodes the WsPacket
 * into raw buff. If the packet is too big, it will be fragmented.
 */
class WsEncoder
{
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
        Code,  ///< for close packet.
        Reason ///< for close packet.
    };

  public:
    explicit WsEncoder(WsTranslayer& trans);

  public:
    /**
     * This function will get the packet from WsTranslayer's packet, and
     * encodes the packet into buffer.
     *
     * @return
     *   ST_Ok         Successfully loaded buffer.
     *   ST_Complete   No packet to encode.
     */
    eCodes loadBuff();

  private:
    /**
     * Enocde packet according to opCode.
     *
     * @return
     *   ST_BufferFull     The send buffer is full.
     *   ST_Complete       The packet has been encoded to buffer.
     */
    eCodes encode();

    /**
     * Encode heartbeat. Heartbeat packet may have payload.
     *
     * @return
     *   ST_BufferFull     The send buffer is full.
     *   ST_Complete       The packet has been encoded to buffer.
     */
    eCodes encodePingPong();

    /**
     * Encode close packet. Close packet has close code and reason.
     *
     * @return
     *   ST_BufferFull     The send buffer is full.
     *   ST_Complete       The packet has been encoded to buffer.
     */
    eCodes encodeClosePacket();

    /**
     * Enocode raw packet or normal data packet.
     *
     * @return
     *   ST_BufferFull     The send buffer is full.
     *   ST_Complete       The packet has been encoded to buffer.
     */
    eCodes encodeDataPacket();

    /**
     * Enocode raw packet. Normally raw packet is sent from other servers,
     * it doesn't need to transfer json to string. So it will be a little
     * faster.
     *
     * @return
     *   ST_BufferFull     The send buffer is full.
     *   ST_Complete       The packet has been encoded to buffer.
     */
    eCodes encodeRawPacket();

    /**
     * Enocode normal data packet. It will encode sysJson, json binary
     * data in sequence.
     *
     * @return
     *   ST_BufferFull     The send buffer is full.
     *   ST_Complete       The packet has been encoded to buffer.
     */
    eCodes encodePlainPacket();

    /**
     * Enocode meta data. Meta data contains data type and data length in
     * in payload.
     *
     * @return
     *   ST_BufferFull     The send buffer is full.
     *   ST_Complete       The packet has been encoded to buffer.
     */
    eCodes encodeMeta();

    /**
     * Enocode data. Encode sysJson/json/binary data.
     *
     * @return
     *   ST_BufferFull     The send buffer is full.
     *   ST_Complete       The packet has been encoded to buffer.
     */
    eCodes encodeData();

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

    /**
     * Compute _totalLen/_payloadLen/_headerLen of normal data packet.
     */
    void computeLength();

    /**
     * Write header info to send buffer.
     *
     * @param firstPkt        If packet is fragmented, this flag marks
     *                        whether the current packet is the first
     *                        fragmented packet. If packet doesn't need
     *                        to be fragmented, it should be true.
     * @param fin             If packet doesn't need fragment, the flag
     *                        should be true. If packet needs to be
     *                        fragmented, the flag marks whether the
     *                        current packet is thet last fragmented
     *                        packet.
     */
    void writeHeader(bool firstPkt, bool fin);

    /**
     * Compute the total length of buffer that the data + meta will take.
     *
     * @param   len    The length of data.
     * @return         The length of data + the length of meta.
     */
    uint64_t getDataLen(uint64_t len);

    /**
     * Mask packet.
     *
     * @param  begin   The start pointer of the buffer that will be masked.
     * @param  end     The pointer of last byte of buffer + 1.
     */
    void maskPacket(unsigned char* begin, unsigned char* end);

    /**
     * Meta info is for the peer to decode what data is it. The format is
     *            | type field (alwasy 1 byte) | data length field |
     * If data length is less than 254 bytes
     *            | type (1 byte) | data length (1 byte) |
     * If data length is >=254 && data length < 65536
     *            | type (1 byte) | 254 (1 byte) | data length (2 bytes) |
     * else
     *            | type (1 byte) | 255 (1 byte) | data length (8 bytes) |
     *
     * @param   item            The type of data item.
     * @param   dataLen         The length of data.
     */
    void getMetaData(ePayloadItem item, uint64_t dataLen);

  private:
    eWriteState _writeState;

    eWriteState _prevWriteState; //< Saves last _writeState.

    // The length of header.
    uint8_t _headerLen;

    // The payload length.
    uint64_t _payloadLen;

    // The member is used to record how many bytes the payload has been encoded.
    // Whe one packet is encoded, it should be equal to _payloadLen.
    uint64_t _payloadEncodedLen = 0;

    // The member is used to record how many bytes the packet has been
    // encoded. When the packet is encoded, the _encodedLen should be
    // equal to _totalLen.
    uint64_t _encodedLen;

    // The total length of the packet payload. If packet is fragmented,
    // _totalLen is the sum of all fragmented packet's payload.
    uint64_t _totalLen;

    // The packet is fragmented?
    bool _fragmented;

    // If the packet will not be fragmented, _firstPacket will be true.
    // Or, if the packet is the first packet of fragmented packet, it will
    // be true, or, it is false.
    bool _firstPacket;

    std::vector<unsigned char>&           _sendVec;
    uint32_t&                             _needSendLen;
    std::list<std::unique_ptr<WsPacket>>& _pktList;

    // The current encoding packet.
    std::unique_ptr<WsPacket> _currPkt;
    const WsConfig&           _config;

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
    unsigned char*             _maskBeginPtr;
    const unsigned char* const _sendVecEndPtr;
    const unsigned char*       _srcPtr;
    const unsigned char*       _srcEndPtr;
};
}

#endif
