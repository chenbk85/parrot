#include "rpcClientConn.h"
#include "wsConfig.h"

namespace parrot
{
RpcClientConn::RpcClientConn(const std::string& wsUrl, const WsConfig& cfg):
    WsClientConn(wsUrl, cfg, false)
{
}
}
