#include "rpcServerConnHandshake.h"
#include "rpcClientConn.h"

namespace parrot
{
RpcServerConnHandshake::RpcServerConnHandshake(RpcServerConn* )
//    : _state(eHandshakeState::RecvCliPubKey), _conn(conn)
{
}

eCodes RpcServerConnHandshake::handshake(WsPacket*, std::unique_ptr<WsPacket>&)
{
    return eCodes::ST_Ok;
}
}
