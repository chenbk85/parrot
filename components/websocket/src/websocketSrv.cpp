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
#include "websocketSrv.h"
#include "macroFuncs.h"
#include "wsConfig.h"

namespace parrot
{
WebSocketSrv::WebSocketSrv(const WsConfig& cfg)
    : TcpServer(),
      TimeoutGuard(),
      _state(WsState::NotOpened),
      _translayer(new WsTranslayer(*this, true, false, cfg)),
      _sentClose(false)
{
    using namespace std::placeholders;
    auto onPktCb = std::bind(&WebSocketSrv::onPacket, this, _1);
    auto onErrCb = std::bind(&WebSocketSrv::onError, this, _1);

    _translayer->registerOnPacketCb(std::move(onPktCb));
    _translayer->registerOnErrorCb(std::move(onErrCb));
}

void WebSocketSrv::onOpen()
{
    PARROT_ASSERT(_state == WsState::NotOpened);
    _state = WsState::Opened;
}

void WebSocketSrv::onPing()
{
    std::unique_ptr<WsPacket> pkt(new WsPacket);
    pkt->setOpCode(eOpCode::Pong);
    sendPacket(pkt);
}

void WebSocketSrv::onPong()
{
    updateTime(std::time(nullptr)); // Update idle check timer.
}

void WebSocketSrv::onData(std::unique_ptr<WsPacket> &&)
{
    updateTime(std::time(nullptr)); // Update idle check timer.

    // TODO:
}

void WebSocketSrv::doClose()
{
    // TODO:
}

void WebSocketSrv::onClose(std::unique_ptr<WsPacket> &&)
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

void WebSocketSrv::onPacket(std::unique_ptr<WsPacket> &&pkt)
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

void WebSocketSrv::onError(eCodes code)
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

void WebSocketSrv::sendPacket(std::unique_ptr<WsPacket>& pkt)
{
    if (_state == WsState::NotOpened)
    {
        LOG_WARN("WebSocketSrv::sendPacket: Websocket has not opened yet. "
                 "Packet will not be sent.");
        return;
    }
    
    if (_state == WsState::Closing)
    {
        LOG_WARN("WebSocketSrv::sendPacket: Websocket is closing. Packet will "
                 "not be sent.");
        return;
    }
    _translayer->sendPacket(pkt);
}

void WebSocketSrv::sendPacket(std::list<std::unique_ptr<WsPacket>>& pktList)
{
    if (_state == WsState::NotOpened)
    {
        LOG_WARN("WebSocketSrv::sendPacket: Websocket has not opened yet. "
                 "Packet will not be sent.");
        return;
    }
    
    if (_state == WsState::Closing)
    {
        LOG_WARN("WebSocketSrv::sendPacket: Websocket is closing. Packet list "
                 "will not be sent.");
        return;
    }
    _translayer->sendPacket(pktList);
}

bool WebSocketSrv::canClose()
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

eIoAction WebSocketSrv::handleIoEvent()
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

void WebSocketSrv::closeWebSocket(std::unique_ptr<WsPacket>& pkt)
{
    sendPacket(pkt);
    _state = WsState::Closing;
    _sentClose = true;
}
}
