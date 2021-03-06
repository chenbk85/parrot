#ifndef __COMPONENT_WEBSOCKET_INC_WSTRANSLAYER_H__
#define __COMPONENT_WEBSOCKET_INC_WSTRANSLAYER_H__

#include <list>
#include <memory>
#include <algorithm> // std::copy
#include <vector>
#include <cstdint>
#include <functional>

#include "codes.h"
#include "wsEncoder.h"
#include "wsServerHandshake.h"
#include "wsDecoder.h"
#include "sysDefinitions.h"

namespace parrot
{
// Forward declarations.
class IoEvent;
class WsPacket;
struct WsConfig;

// WsTranslayer implements Websocket implement.
class WsTranslayer
{
    friend class WsServerHandshake;
    friend class WsClientHandshake;
    friend class WsEncoder;
    friend class WsDecoder;

  protected:
    enum eTranslayerState
    {
        RecvHttpHandshake,
        SendHttpHandshake,
        WsConnected,
        WsError // In this state, we only send packets.
    };

  public:
    WsTranslayer(IoEvent& io,
                 bool recvMasked,
                 bool sendMsked,
                 const WsConfig& cfg);
    virtual ~WsTranslayer() = default;
    WsTranslayer(const WsTranslayer&) = delete;
    WsTranslayer& operator=(const WsTranslayer&) = delete;

  public:
    void setRandom(MtRandom* random);
    // Callbacks.
    void registerOnOpenCb(std::function<void()>&& cb);
    void
    registerOnPacketCb(std::function<void(std::unique_ptr<WsPacket>&&)>&& cb);
    void registerOnErrorCb(std::function<void(eCodes)>&& cb);

  public:
    void sendPacket(std::list<std::unique_ptr<WsPacket>>& pktList);
    void sendPacket(std::unique_ptr<WsPacket>& pkt);
    void close();
    bool canSwitchToSend() const;
    bool isAllSent() const;


  public:
    virtual eIoAction work(eIoAction evt) = 0;
    
  protected:
    std::vector<char>* prepareDataToSend();
    eCodes recvData();
    eCodes sendData();

  protected:
    IoEvent& _io;
    bool _needRecvMasked;
    bool _needSendMasked;
    std::list<std::unique_ptr<WsPacket>> _pktList;

    std::unique_ptr<WsDecoder> _wsDecoder;

    // Send buffer.
    std::vector<unsigned char> _sendVec;

    uint32_t _needSendLen;
    // How many bytes are left in _sendVec or _sendFragmentedVec. There
    // will be only one vector of the above two vectors contains data at
    // one time.
    uint32_t _sentLen;

    std::vector<unsigned char> _recvVec;
    uint32_t _rcvdLen;

    std::unique_ptr<WsEncoder> _wsEncoder;

    std::function<void()> _onOpenCb;
    std::function<void(std::unique_ptr<WsPacket>&&)> _onPacketCb;
    std::function<void(eCodes)> _onErrorCb;

    // For SslIo, when send/recv of SslIo returns RetryWhenReadable, we
    // should stop sending, and wait for read signal.
    bool _canSend;
    MtRandom* _random;
    const WsConfig& _config;
};
}

#endif
