#include "rpcServerConn.h"
#include "wsConfig.h"

namespace parrot
{
RpcServerConn::RpcServerConn(const WsConfig& cfg) : WsServerConn(cfg, false)
{
}
}
