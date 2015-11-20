#include <ctime>
#include <iostream>
#include <functional>

#include "mtRandom.h"
#include "logger.h"
#include "json.h"
#include "wsHttpResponse.h"
#include "wsTranslayer.h"
#include "wsPacket.h"
#include "wsServerConn.h"
#include "macroFuncs.h"
#include "wsConfig.h"
#include "wsPacketHandler.h"

namespace parrot
{
WsServerConn::WsServerConn(const WsConfig& cfg)
    : TcpServerConn(),
      TimeoutGuard(),
      DoubleLinkedListNode<WsServerConn>(),
      _state(eWsState::NotOpened),
      _pktHandler(nullptr),
      _session(new Session()),
      _translayer(new WsTranslayer(*this, true, false, cfg)),
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

void WsServerConn::setPacketHandler(PacketHandler *hdr)
{
    _pktHandler = hdr;
}

void WsServerConn::setRandom(MtRandom *r)
{
    _translayer->setRandom(r);
}

bool WsServerConn::canSwitchToSend() const
{
    return _translayer->canSwitchToSend();
}

std::shared_ptr<Session>& WsServerConn::getSession()
{
    return _session;
}

void WsServerConn::onOpen()
{
    PARROT_ASSERT(_state == eWsState::NotOpened);
    _state = eWsState::Opened;
}

void WsServerConn::onPing()
{
    std::unique_ptr<WsPacket> pkt(new WsPacket);
    pkt->setOpCode(eOpCode::Pong);
    sendPacket(pkt);
}

void WsServerConn::onPong()
{
}

void WsServerConn::onData(std::unique_ptr<WsPacket>&& pkt)
{
    std::cout << "WsServerConn::onData" << std::endl;
    _pktHandler->onPacket(_session, std::move(pkt));
}

void WsServerConn::doClose()
{
}

void WsServerConn::onClose(std::unique_ptr<WsPacket>&& p)
{
    _state = eWsState::Closing;
    p->decode();

    LOG_DEBUG("WsServerConn::onClose: Close code is "
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

void WsServerConn::onPacket(std::unique_ptr<WsPacket>&& pkt)
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

void WsServerConn::onError(eCodes code)
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

void WsServerConn::sendPacket(std::unique_ptr<WsPacket>& pkt)
{
    if (_state == eWsState::NotOpened)
    {
        LOG_WARN("WsServerConn::sendPacket: Websocket has not opened yet. "
                 "Packet will not be sent.");
        return;
    }

    if (_state == eWsState::Closing)
    {
        LOG_WARN("WsServerConn::sendPacket: Websocket is closing. Packet will "
                 "not be sent.");
        return;
    }
    _translayer->sendPacket(pkt);
}

void WsServerConn::sendPacket(std::list<std::unique_ptr<WsPacket>>& pktList)
{
    if (_state == eWsState::NotOpened)
    {
        LOG_WARN("WsServerConn::sendPacket: Websocket has not opened yet. "
                 "Packet will not be sent.");
        return;
    }

    if (_state == eWsState::Closing)
    {
        LOG_WARN("WsServerConn::sendPacket: Websocket is closing. Packet list "
                 "will not be sent.");
        return;
    }
    _translayer->sendPacket(pktList);
}

bool WsServerConn::canClose()
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

eIoAction WsServerConn::handleIoEvent()
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

void WsServerConn::closeWebSocket(std::unique_ptr<WsPacket>& pkt)
{
    sendPacket(pkt);
    _state     = eWsState::Closing;
    _sentClose = true;
}
}
