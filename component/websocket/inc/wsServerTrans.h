#ifndef __COMPONENT_WEBSOCKET_INC_WSSERVERTRANS_H__
#define __COMPONENT_WEBSOCKET_INC_WSSERVERTRANS_H__

#include <memory>

#include "sysDefinitions.h"
#include "wsTranslayer.h"
#include "wsServerHandshake.h"

namespace parrot
{
class IoEvent;
struct WsConfig;

class WsServerTrans : public WsTranslayer
{
  public:
    WsServerTrans(IoEvent& io,
                  bool recvMasked,
                  bool sendMasked,
                  const WsConfig& cfg);

  public:
    eIoAction work(eIoAction evt) override;

  private:
    eTranslayerState _state;    
    std::unique_ptr<WsServerHandshake> _handshake;
};
}

#endif
