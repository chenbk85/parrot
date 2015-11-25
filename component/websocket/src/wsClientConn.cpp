#include <ctime>
#include <iostream>
#include <functional>

#include "mtRandom.h"
#include "logger.h"
#include "json.h"
#include "wsHttpResponse.h"
#include "wsTranslayer.h"
#include "wsPacket.h"
#include "wsClientConn.h"
#include "macroFuncs.h"
#include "wsConfig.h"
#include "wsPacketHandler.h"

namespace parrot
{
WsClientConn::WsClientConn(const string& ip,
                           uint16_t port,
                           const WsConfig& cfg,
                           bool sendMasked)
    : TcpServerConn(),
      TimeoutGuard(),
      DoubleLinkedListNode<WsClientConn>(),
      _state(eWsState::NotOpened),
      _pktHandler(nullptr),
      _session(new Session()),
      _translayer(new WsTranslayer(*this, false, sendMasked, cfg)),
      _sentClose(false)
{
    using namespace std::placeholders;

    auto onOpenCb = std::bind(&WsClientConn::onOpen, this);
    auto onPktCb  = std::bind(&WsClientConn::onPacket, this, _1);
    auto onErrCb  = std::bind(&WsClientConn::onError, this, _1);

    _translayer->registerOnOpenCb(std::move(onOpenCb));
    _translayer->registerOnPacketCb(std::move(onPktCb));
    _translayer->registerOnErrorCb(std::move(onErrCb));

    setRemotePort(port);
    setRemoteAddr(ip);
}

void WsClientConn::setPacketHandler(PacketHandler* hdr)
{
    _pktHandler = hdr;
}

void WsClientConn::setRandom(MtRandom* r)
{
    _translayer->setRandom(r);
}

bool WsClientConn::canSwitchToSend() const
{
    return _translayer->canSwitchToSend();
}

std::shared_ptr<Session>& WsClientConn::getSession()
{
    return _session;
}

void WsClientConn::onOpen()
{
    PARROT_ASSERT(_state == eWsState::NotOpened);
    _state = eWsState::Opened;
}

void WsClientConn::onPing()
{
    std::unique_ptr<WsPacket> pkt(new WsPacket);
    pkt->setOpCode(eOpCode::Pong);
    sendPacket(pkt);
}

void WsClientConn::onPong()
{
}

void WsClientConn::onData(std::unique_ptr<WsPacket>&& pkt)
{
    std::cout << "WsClientConn::onData" << std::endl;
    _pktHandler->onPacket(_session, std::move(pkt));
}

void WsClientConn::doClose()
{
}

void WsClientConn::onClose(std::unique_ptr<WsPacket>&& p)
{
    _state = eWsState::Closing;
    p->decode();

    LOG_DEBUG("WsClientConn::onClose: Close code is "
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

void WsClientConn::onPacket(std::unique_ptr<WsPacket>&& pkt)
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

void WsClientConn::onError(eCodes code)
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

void WsClientConn::sendPacket(std::unique_ptr<WsPacket>& pkt)
{
    if (_state == eWsState::NotOpened)
    {
        LOG_WARN("WsClientConn::sendPacket: Websocket has not opened yet. "
                 "Packet will not be sent.");
        return;
    }

    if (_state == eWsState::Closing)
    {
        LOG_WARN("WsClientConn::sendPacket: Websocket is closing. Packet will "
                 "not be sent.");
        return;
    }
    _translayer->sendPacket(pkt);
}

void WsClientConn::sendPacket(std::list<std::unique_ptr<WsPacket>>& pktList)
{
    if (_state == eWsState::NotOpened)
    {
        LOG_WARN("WsClientConn::sendPacket: Websocket has not opened yet. "
                 "Packet will not be sent.");
        return;
    }

    if (_state == eWsState::Closing)
    {
        LOG_WARN("WsClientConn::sendPacket: Websocket is closing. Packet list "
                 "will not be sent.");
        return;
    }
    _translayer->sendPacket(pktList);
}

bool WsClientConn::canClose()
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

eIoAction WsClientConn::handleIoEvent()
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

void WsClientConn::closeWebSocket(std::unique_ptr<WsPacket>& pkt)
{
    sendPacket(pkt);
    _state     = eWsState::Closing;
    _sentClose = true;
}

void WsClientConn::work()
{
    switch (_state)
    {
        case eWsState::NotOpened:
        {
            connect(_remoteIP, _remotePort);

            if (isConnected)
            {
                _state = eWsState::Connected;
            }
            else
            {
                _state = eWsState::Connecting;
            }
        }
        break;

        case eWsState::Connecting:
        {

        }
        break;
        
        case eWsState::Connected:
        {

        }
        break;
    }
}
}
