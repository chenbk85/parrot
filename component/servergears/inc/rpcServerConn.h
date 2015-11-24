#ifndef __COMPONENT_SERVERGEAR_INC_RPCSERVERCONN_H__
#define __COMPONENT_SERVERGEAR_INC_RPCSERVERCONN_H__

#include "wsServerConn.h"

namespace parrot
{
struct WsConfig;

class RpcServerConn : public WsServerConn
{
  public:
    RpcServerConn(const WsConfig& cfg);
};
}

#endif
