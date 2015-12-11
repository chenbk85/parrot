#ifndef __COMPONENT_SERVERGEAR_INC_RPCSERVERCONN_H__
#define __COMPONENT_SERVERGEAR_INC_RPCSERVERCONN_H__

#include "wsServerConn.h"
#include "rpcSession.h"

namespace parrot
{
struct WsConfig;

class RpcServerConn : public WsServerConn<RpcSession>
{
  public:
    RpcServerConn(const WsConfig& cfg);
};
}

#endif
