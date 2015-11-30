#ifndef __COMPONENT_WEBSOCKET_INC_WSCLIENTTRANS_H__
#define __COMPONENT_WEBSOCKET_INC_WSCLIENTTRANS_H__

#include <memory>

#include "sysDefinition.h"
#include "wsTranslayer.h"
#include "wsClientHandshake.h"

namespace parrot
{
class IoEvent;
struct WsConfig;

class WsClientTrans : public WsTranslayer
{
  public:
    WsClientTrans(IoEvent& io,
                  bool recvMasked,
                  bool sendMasked,
                  const WsConfig& cfg);

  public:
    eIoAction work(eIoAction evt) override;

  private:
    eTranslayerState _state;    
    std::unqiue_ptr<WsClientHandshake> _handshake;
};
}

#endif
