#include <ctime>
#include <functional>

#include "mtRandom.h"
#include "logger.h"
#include "json.h"
#include "wsEncoder.h"
#include "wsParser.h"
#include "wsHttpResponse.h"
#include "wsTranslayer.h"
#include "wsPacket.h"
#include "wsServerConn.h"
#include "macroFuncs.h"
#include "wsConfig.h"

namespace parrot
{
WsServerConn::WsServerConn(const WsConfig& cfg)
    : TcpServer(),
      TimeoutGuard(),
      _state(WsState::NotOpened),
      _translayer(new WsTranslayer(*this, true, false, cfg)),
      _sentClose(false)
{
    using namespace std::placeholders;
    auto onPktCb = std::bind(&WsServerConn::onPacket, this, _1);
    auto onErrCb = std::bind(&WsServerConn::onError, this, _1);

    _translayer->registerOnPacketCb(std::move(onPktCb));
    _translayer->registerOnErrorCb(std::move(onErrCb));
}

void WsServerConn::onOpen()
{
    PARROT_ASSERT(_state == WsState::NotOpened);
    _state = WsState::Opened;
}

void WsServerConn::onPing()
{
    std::unique_ptr<WsPacket> pkt(new WsPacket);
    pkt->setOpCode(eOpCode::Pong);
    sendPacket(pkt);
}

void WsServerConn::onPong()
{
    updateTime(std::time(nullptr)); // Update idle check timer.
}

void WsServerConn::onData(std::unique_ptr<WsPacket> &&)
{
    updateTime(std::time(nullptr)); // Update idle check timer.

    // TODO:
}

void WsServerConn::doClose()
{
    // TODO:
}

void WsServerConn::onClose(std::unique_ptr<WsPacket> &&)
{
    _state = WsState::Closing;
    
    if (_sentClose && _translayer->isAllSent())
    {
        // TODO: Notify up layer.
        // uplayer->onClose(p->getErrorCode())
       doClose();
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

void WsServerConn::onPacket(std::unique_ptr<WsPacket> &&pkt)
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
    _state = WsState::Closing;
}

void WsServerConn::sendPacket(std::unique_ptr<WsPacket>& pkt)
{
    if (_state == WsState::NotOpened)
    {
        LOG_WARN("WsServerConn::sendPacket: Websocket has not opened yet. "
                 "Packet will not be sent.");
        return;
    }
    
    if (_state == WsState::Closing)
    {
        LOG_WARN("WsServerConn::sendPacket: Websocket is closing. Packet will "
                 "not be sent.");
        return;
    }
    _translayer->sendPacket(pkt);
}

void WsServerConn::sendPacket(std::list<std::unique_ptr<WsPacket>>& pktList)
{
    if (_state == WsState::NotOpened)
    {
        LOG_WARN("WsServerConn::sendPacket: Websocket has not opened yet. "
                 "Packet will not be sent.");
        return;
    }
    
    if (_state == WsState::Closing)
    {
        LOG_WARN("WsServerConn::sendPacket: Websocket is closing. Packet list "
                 "will not be sent.");
        return;
    }
    _translayer->sendPacket(pktList);
}

bool WsServerConn::canClose()
{
    if (_state != WsState::Closing)
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
    _state = WsState::Closing;
    _sentClose = true;
}
}
