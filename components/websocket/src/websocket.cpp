#include "websocket.h"
#include "macroFuncs.h"

namespace parrot
{
    WebSocket::WebSocket(PktHandler &hdr):
        IoEvent(),
        _pktHandler(hdr),
        _translayer(new WsTranslayer(this))
    {
    }

    WebSocket::~WebSocket()
    {
        delete _translayer;
        _translayer = nullptr;
    }

    eIoAction WebSocket::heartbeat()
    {

    }

    void WebSocket::onPacket(std::unique_ptr<WsPacket> &&pkt)
    {
        _pktHandler.onPacket(std::move(pkt));
    }

    eIoAction WebSocket::sendPacket(std::unique_ptr<WsPacket> &&pkt)
    {
        return _translayer->sendPacket(std::move(pkt));
    }

    eIoAction WebSocket::sendPacket(
        std::list<std::unique_ptr<WsPacket>> &&pktList)
    {
        return _translayer->sendPacket(std::move(pktList));
    }

    eIoAction WebSocket::handleIoEvent()
    {
        if (isError())
        {
            _pktHandler.onError();
            return eIoAction::Remove;
        }

        if (isEof())
        {
            _pktHandler.onRemoteDisconnect();
            return eIoAction::Remove;
        }

        if (isReadAvail())
        {
            return _translayer->recv();
        }

        if (isWriteAvail())
        {
            return _translayer->send();
        }

        PARROT_ASSERT(false);
        return eIoAction::None;
    }

    eIoAction WebSocket::closeWebSocket()
    {
        
    }
}
