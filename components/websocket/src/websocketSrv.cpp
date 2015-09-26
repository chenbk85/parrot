#include "json.h"
#include "wsTranslayer.h"
#include "wsPacket.h"
#include "websocketSrv.h"
#include "macroFuncs.h"
#include "wsConfig.h"

namespace parrot {
WebSocketSrv::WebSocketSrv(const WsConfig &cfg)
    : TcpServer(), TimeoutGuard(),
      _translayer(new WsTranslayer<WebSocketSrv>(*this, true, cfg)) {
}

void WebSocketSrv::onOpen() {
}

void WebSocketSrv::onPong() {
}

void WebSocketSrv::onData(WsParser::eOpCode, std::vector<char>::iterator,
                          std::vector<char>::iterator) {
}

void WebSocketSrv::sendPacket(std::unique_ptr<WsPacket> &pkt) {
    _translayer->sendPacket(pkt);
}

void WebSocketSrv::sendPacket(std::list<std::unique_ptr<WsPacket>> &pktList) {
    _translayer->sendPacket(pktList);
}

eIoAction WebSocketSrv::handleIoEvent() {
    if (isError()) {
        return eIoAction::Remove;
    }

    if (isEof()) {
        return eIoAction::Remove;
    }

    if (isReadAvail()) {
        return _translayer->work(eIoAction::Read);
    }

    if (isWriteAvail()) {
        return _translayer->work(eIoAction::Write);
    }

    PARROT_ASSERT(false);
    return eIoAction::None;
}

void WebSocketSrv::closeWebSocket(const std::string &) {
}
}
