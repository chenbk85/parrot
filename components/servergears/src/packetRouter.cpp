#include "packetRouter.h"
#include "logger.h"
#include "wsServerConn.h"
#include "wsPacket.h"

namespace parrot
{
bool PacketRouter::handle(WsServerConn* connPtr,
                          std::unique_ptr<WsPacket>&& pkt)
{
    if (!pkt->decode())
    {
        LOG_WARN("PacketRouter::handle: Failed to decode packet. Remote is "
                 << connPtr->getRemoteAddr() << ".");
        return false;
    }

    
}
}
