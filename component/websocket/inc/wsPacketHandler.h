#ifndef __COMPONENT_WEBSOCKET_INC_WSPACKETHANDLER_H__
#define __COMPONENT_WEBSOCKET_INC_WSPACKETHANDLER_H__

#include <memory>
#include "codes.h"

namespace parrot
{
class WsPacket;

template <typename Sess, typename Conn> class WsPacketHandler
{
  public:
    virtual ~WsPacketHandler() = default;

  public:
    virtual void onPacket(Conn* conn,
                          std::unique_ptr<WsPacket>&&) = 0;
    virtual void onClose(Conn* conn,
                         std::unique_ptr<WsPacket>&&) = 0;
};
}

#endif
