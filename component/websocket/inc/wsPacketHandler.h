#ifndef __COMPONENT_WEBSOCKET_INC_WSPACKETHANDLER_H__
#define __COMPONENT_WEBSOCKET_INC_WSPACKETHANDLER_H__

#include <memory>
#include "codes.h"

namespace parrot
{

template<typename Sess, typename Conn>
class WsPacketHandler
{
  public:
    virtual ~WsPacketHandler() = default;

  public:
    virtual void onPacket(std::shared_ptr<const Sess>&&,
                          std::unique_ptr<WsPacket>&&) = 0;
    virtual void onClose(Conn* conn, eCodes) = 0;
};
}

#endif
