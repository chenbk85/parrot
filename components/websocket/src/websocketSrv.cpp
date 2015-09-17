#include "websocket.h"
#include "macroFuncs.h"

namespace parrot
{
    WebSocketSrv::WebSocketSrv():
        TcpServer(),
        TimeoutGuard(),
        _translayer(new WsTranslayer<WebSocketSrv>(this, true))
    {
    }

    void WebSocketSrv::onOpen()
    {

    }

    void WebSocketSrv::onPong()
    {

    }

    void WebSocketSrv::onData(
        const std::vector<char>::iterator &,
        const std::vector<char>::iterator &)
    {
    }

    eIoAction WebSocketSrv::sendPacket(std::unique_ptr<WsPacket> &&pkt)
    {
        return _translayer->sendPacket(std::move(pkt));
    }

    eIoAction WebSocketSrv::sendPacket(
        std::list<std::unique_ptr<WsPacket>> &&pktList)
    {
        return _translayer->sendPacket(std::move(pktList));
    }

    eIoAction WebSocketSrv::handleIoEvent()
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
            return _translayer->work(IoEvent::Read);
        }

        if (isWriteAvail())
        {
            return _translayer->work(IoEvent::Write);
        }

        PARROT_ASSERT(false);
        return eIoAction::None;
    }

    eIoAction WebSocketSrv::closeWebSocket(const std::string &)
    {
        return eIoAction::None;
    }
}
