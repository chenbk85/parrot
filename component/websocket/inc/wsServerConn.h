#ifndef __COMPONENT_WEBSOCKET_INC_WSSERVERCONN_H__
#define __COMPONENT_WEBSOCKET_INC_WSSERVERCONN_H__

#include <memory>
#include <list>
#include <vector>
#include <string>
#include <ctime>
#include <iostream>
#include <functional>

#include "codes.h"
#include "tcpServerConn.h"
#include "timeoutGuard.h"
#include "doubleLinkedListNode.h"
#include "mtRandom.h"
#include "logger.h"
#include "json.h"
#include "macroFuncs.h"

#include "wsServerTrans.h"
#include "wsDefinition.h"
#include "wsPacketHandler.h"
#include "wsPacket.h"
#include "wsConfig.h"

namespace parrot
{
class WsPacket;
struct WsConfig;
struct UrlInfo;

template <class Sess>
class WsServerConn : public TcpServerConn,
                     public TimeoutGuard,
                     public DoubleLinkedListNode<WsServerConn<Sess>>
{
    using PacketHandler = WsPacketHandler<Sess, WsServerConn>;

    enum class eWsState
    {
        NotOpened,
        Opened,
        Closing
    };

  public:
    WsServerConn(const WsConfig& cfg, bool recvMasked = true);
    virtual ~WsServerConn() = default;
    WsServerConn(const WsServerConn&) = delete;
    WsServerConn& operator=(const WsServerConn&) = delete;

  public:
    void setPacketHandler(PacketHandler* hdr);
    void bindSession(std::shared_ptr<Sess>&);
    std::shared_ptr<Sess>& getSession();
    void sendPacket(std::unique_ptr<WsPacket>& pkt);
    void sendPacket(std::list<std::unique_ptr<WsPacket>>& pkt);
    eIoAction handleIoEvent() override;
    // Up layer close the socket, we should sent the close packet and
    // disconnect the connection.
    void closeWebSocket(std::unique_ptr<WsPacket>& pkt);
    void setRandom(MtRandom* random);
    bool canSwitchToSend() const;

  private:
    void onError(eCodes c);
    void onPacket(std::unique_ptr<WsPacket>&& pkt);

    void onOpen();
    void onPing();
    void onPong();
    void onClose(std::unique_ptr<WsPacket>&& pkt);
    void onData(std::unique_ptr<WsPacket>&& pkt);
    bool canClose();
    void doClose();

  private:
    eWsState _state;
    PacketHandler* _pktHandler;
    std::shared_ptr<Sess> _session;
    std::unique_ptr<WsServerTrans> _translayer;
    bool _sentClose;
};

template <class Sess>
WsServerConn<Sess>::WsServerConn(const WsConfig& cfg, bool recvMasked)
    : TcpServerConn(),
      TimeoutGuard(),
      DoubleLinkedListNode<WsServerConn>(),
      _state(eWsState::NotOpened),
      _pktHandler(nullptr),
      _session(),
      _translayer(new WsServerTrans(*this, recvMasked, false, cfg)),
      _sentClose(false)
{
    using namespace std::placeholders;

    auto onOpenCb = std::bind(&WsServerConn::onOpen, this);
    auto onPktCb  = std::bind(&WsServerConn::onPacket, this, _1);
    auto onErrCb  = std::bind(&WsServerConn::onError, this, _1);

    _translayer->registerOnOpenCb(std::move(onOpenCb));
    _translayer->registerOnPacketCb(std::move(onPktCb));
    _translayer->registerOnErrorCb(std::move(onErrCb));
}

template <class Sess>
void WsServerConn<Sess>::setPacketHandler(PacketHandler* hdr)
{
    _pktHandler = hdr;
}

template <class Sess> void WsServerConn<Sess>::setRandom(MtRandom* r)
{
    _translayer->setRandom(r);
}

template <class Sess> bool WsServerConn<Sess>::canSwitchToSend() const
{
    return _translayer->canSwitchToSend();
}

template <class Sess> std::shared_ptr<Sess>& WsServerConn<Sess>::getSession()
{
    return _session;
}

template <class Sess>
void WsServerConn<Sess>::bindSession(std::shared_ptr<Sess>& sess)
{
    _session = sess;
}

template <class Sess> void WsServerConn<Sess>::onOpen()
{
    PARROT_ASSERT(_state == eWsState::NotOpened);
    _state = eWsState::Opened;
}

template <class Sess> void WsServerConn<Sess>::onPing()
{
    std::unique_ptr<WsPacket> pkt(new WsPacket);
    pkt->setOpCode(eOpCode::Pong);
    sendPacket(pkt);
}

template <class Sess> void WsServerConn<Sess>::onPong()
{
}

template <class Sess>
void WsServerConn<Sess>::onData(std::unique_ptr<WsPacket>&& pkt)
{
    _pktHandler->onPacket(_session, std::move(pkt));
}

template <class Sess> void WsServerConn<Sess>::doClose()
{
}

template <class Sess>
void WsServerConn<Sess>::onClose(std::unique_ptr<WsPacket>&& p)
{
    _state = eWsState::Closing;
    p->decode();

    LOG_DEBUG("WsServerConn<Sess>::onClose: Close code is "
              << p->getCloseCode() << ". Reason is " << p->getCloseReason()
              << ".");

    if (_sentClose && _translayer->isAllSent())
    {
        doClose();
        _pktHandler->onClose(this, std::move(p));
    }
    else
    {
        // Closing hand wave.
        std::unique_ptr<WsPacket> pkt(new WsPacket);
        pkt->setClose(eCodes::WS_NormalClosure);
        sendPacket(pkt);
        _sentClose = true;
    }
}

template <class Sess>
void WsServerConn<Sess>::onPacket(std::unique_ptr<WsPacket>&& pkt)
{
    switch (pkt->getOpCode())
    {
        case eOpCode::Binary:
        {
            onData(std::move(pkt));
        }
        break;

        case eOpCode::Ping:
        {
            onPing();
        }
        break;

        case eOpCode::Pong:
        {
            onPong();
        }
        break;

        case eOpCode::Close:
        {
            onClose(std::move(pkt));
        }
        break;

        default:
        {
            PARROT_ASSERT(false);
        }
        break;
    }
}

template <class Sess> void WsServerConn<Sess>::onError(eCodes code)
{
    // WS_ProtocolError
    // WS_Unsupporteddata
    // WS_InvalidFramePayloadData
    // WS_PolicyViolation
    // WS_MessageTooBig
    // WS_InternalError

    std::unique_ptr<WsPacket> pkt(new WsPacket());
    pkt->setClose(code);
    sendPacket(pkt);
    _sentClose = true;
    _state     = eWsState::Closing;
}

template <class Sess>
void WsServerConn<Sess>::sendPacket(std::unique_ptr<WsPacket>& pkt)
{
    if (_state == eWsState::NotOpened)
    {
        LOG_WARN(
            "WsServerConn<Sess>::sendPacket: Websocket has not opened yet. "
            "Packet will not be sent.");
        return;
    }

    if (_state == eWsState::Closing)
    {
        LOG_WARN(
            "WsServerConn<Sess>::sendPacket: Websocket is closing. Packet will "
            "not be sent.");
        return;
    }
    _translayer->sendPacket(pkt);
}

template <class Sess>
void WsServerConn<Sess>::sendPacket(
    std::list<std::unique_ptr<WsPacket>>& pktList)
{
    if (_state == eWsState::NotOpened)
    {
        LOG_WARN(
            "WsServerConn<Sess>::sendPacket: Websocket has not opened yet. "
            "Packet will not be sent.");
        return;
    }

    if (_state == eWsState::Closing)
    {
        LOG_WARN(
            "WsServerConn<Sess>::sendPacket: Websocket is closing. Packet list "
            "will not be sent.");
        return;
    }
    _translayer->sendPacket(pktList);
}

template <class Sess> bool WsServerConn<Sess>::canClose()
{
    if (_state != eWsState::Closing)
    {
        return false;
    }

    if (_sentClose && _translayer->isAllSent())
    {
        // Closing hand wave finished.
        doClose();
        return true;
    }

    return false;
}

template <class Sess> eIoAction WsServerConn<Sess>::handleIoEvent()
{
    if (isError())
    {
        // Todo: Notifier uplayer
        return eIoAction::Remove;
    }

    if (isEof())
    {
        // Todo: Notifier uplayer
        return eIoAction::Remove;
    }

    eIoAction act;

    if (isReadAvail())
    {
        act = _translayer->work(eIoAction::Read);
        if (canClose())
        {
            return eIoAction::Remove;
        }
        return act;
    }

    if (isWriteAvail())
    {
        act = _translayer->work(eIoAction::Write);
        if (canClose())
        {
            return eIoAction::Remove;
        }
        return act;
    }

    PARROT_ASSERT(false);
    return eIoAction::None;
}

template <class Sess>
void WsServerConn<Sess>::closeWebSocket(std::unique_ptr<WsPacket>& pkt)
{
    sendPacket(pkt);
    _state     = eWsState::Closing;
    _sentClose = true;
}
}

#endif
