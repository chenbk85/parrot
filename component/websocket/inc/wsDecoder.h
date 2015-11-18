#ifndef __COMPONENT_WEBSOCKET_INC_WSPARSER_H__
#define __COMPONENT_WEBSOCKET_INC_WSPARSER_H__

#include <vector>
#include <string>
#include <functional>
#include <cstdint>
#include <array>
#include "codes.h"
#include "wsDefinition.h"

namespace parrot
{
struct WsConfig;
class WsPacket;
class WsTranslayer;

// WsDecoder is the websocket parser for both server side and
// client side. RPC packet parser will also use this class.
class WsDecoder
{
  public:
    enum eParseState
    {
        Begin,
        ParsingHeader,
        ParsingBody
    };

  private:
    using VecCharIt    = std::vector<unsigned char>::iterator;
    using CallbackFunc = std::function<void(std::unique_ptr<WsPacket>&&)>;

  public:
    explicit WsDecoder(WsTranslayer &trans);
    ~WsDecoder() = default;
    WsDecoder(const WsDecoder&) = delete;
    WsDecoder& operator=(const WsDecoder&) = delete;

  public:
    // getResult
    //
    // Get the parsing result.
    //
    // Return
    //  * WS_UnsupportedData
    //  * WS_ProtocolError
    //  * WS_MessageTooBig
    //  * ST_Ok
    eCodes getResult() const
    {
        return _parseResult;
    }

    // parse
    //
    // Parse the buffer, if received more than one packets,
    // it will continue to parse. E.g. If we received 2 packets
    // and only parts of 3rd packet, it will parse the first two
    // packet, then, move the bytes of 3rd packet to the begin
    // of the buffer, and reset the recvVec size to the received
    // length of 3rd packet.
    //
    // return
    //  * ST_Complete     Parsing completed.
    //  * ST_NeedRecv     Need receive more data to parse.
    eCodes parse();

  private:
    // isFin
    //
    // Fin is the flag used to determine whether the whole
    // packet is received. If a big packet is fragmented to
    // several packets, the fragmented packets will be sent
    // in sequence. And the last fragmented packet's fin
    // flag must be set to 1. Other's fin must be set to 0.
    // If the packet is not fragmented, the fin flag must
    // be 1.
    //
    // return:
    //  If fin flag is 1, returns true.
    bool isFin() const
    {
        return _fin;
    }

    // doParse
    //
    // This function parses the buffer and invokes the
    // callback function if parsing successfully completed.
    // The client must invoke the 'getResult' to retrieve
    // the parsing result.
    //
    // return:
    //  * ST_NeedRecv    // Hasn't receive the full packet,
    //                        need receive more.
    //  * ST_Ok          // Finished parsing.
    eCodes doParse();

    // parseHeader
    //
    // Parses the websocket header. Get the length of payload
    // and mask key if exists.
    void parseHeader();

    // parseBody
    //
    // Unmasking the payload. If the packet is fragmented, and
    // hasn't receive all, saves it. If received all or not
    // fragmented, invoke the callback to notify uplayer.
    void parseBody();

    void unmaskData();

    std::unique_ptr<WsPacket>
    createWsPacket(const std::vector<unsigned char>::iterator& begin,
                   const std::vector<unsigned char>::iterator& end);

  private:
    std::vector<unsigned char>& _recvVec;
    uint32_t& _rcvdLen;
    const std::string& _remoteIp;
    bool _needMask;
    CallbackFunc& _callbackFunc;
    const WsConfig& _config;

    eParseState _state;
    bool _fin;
    bool _masked;
    eOpCode _opCode;
    eOpCode _fragmentDataType;
    uint64_t _lastParsePos;
    uint64_t _pktBeginPos;
    uint8_t _headerLen;
    uint64_t _payloadLen;
    uint32_t _maskingKey;
    eCodes _parseResult;
    std::vector<unsigned char> _packetVec;
    uint32_t _largePktCount;
};
}
#endif
