#ifndef __COMPONENTS_WEBSOCKET_INC_WSTRANSLAYER_H__
#define __COMPONENTS_WEBSOCKET_INC_WSTRANSLAYER_H__

#include <list>
#include <memory>
#include <algorithm> // std::copy
#include <vector>
#include <cstdint>
#include <functional>

#include "codes.h"

namespace parrot
{
// Forward declarations.
class MtRandom;
class WsHttpResponse;
class IoEvent;
class WsPacket;
class WsParser;
struct WsConfig;
enum class eIoAction : uint8_t;

// WsTranslayer implements Websocket implement.
class WsTranslayer
{
  private:
    enum eTranslayerState
    {
        RecvHttpHandshake,
        SendHttpHandshake,
        WsConnected
    };

  public:
    WsTranslayer(IoEvent& io,
                 bool recvMasked,
                 bool sendMsked,
                 const WsConfig& cfg);
    ~WsTranslayer() = default;
    WsTranslayer(const WsTranslayer&) = delete;
    WsTranslayer& operator=(const WsTranslayer&) = delete;

  public:
    // Callbacks.
    void registerOnPacketCb(
        std::function<void(std::unique_ptr<WsPacket>&&)>&& cb);
    void registerOnErrorCb(std::function<void(eCodes)>&& cb);

  public:
    void sendPacket(std::list<std::unique_ptr<WsPacket>>& pktList);
    void sendPacket(std::unique_ptr<WsPacket>& pkt);
    eIoAction work(eIoAction evt);
    void close();
    bool isAllSent() const;

  private:
    std::vector<char>* prepareDataToSend();
    eCodes recvData();
    eCodes sendData();

  private:
    eTranslayerState _state;
    IoEvent& _io;
    bool _needRecvMasked;
    bool _needSendMasked;
    std::list<std::unique_ptr<WsPacket>> _pktList;

    std::unique_ptr<WsHttpResponse> _httpRsp;
    std::unique_ptr<WsParser> _wsParser;

    // If packet length is greater than the capacity of _sendVec, the
    // packet will be fragmented into _sendFragmentedVec. After sent
    // the fragmented packet, the _sendFragmentedVec will set shrink to
    // empty. _sendVec will never shrink.
    std::vector<char> _sendVec;
    std::vector<char> _sendFragmentedVec;

    // How many bytes are left in _sendVec or _sendFragmentedVec. There
    // will be only one vector of the above two vectors contains data at
    // one time.
    uint32_t _sentLen;

    std::vector<char> _recvVec;

    std::unique_ptr<WsEncoder> _wsEncoder;

    std::function<void(std::unique_ptr<WsPacket> &&)> _onPacketCb;
    std::function<void(eCodes)> _onErrorCb;

    std::unique_ptr<MtRandom> _random;
    const WsConfig& _config;
};
}

#endif
