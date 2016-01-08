#include "rpcClientConnHandshake.h"
#include "macroFuncs.h"

namespace parrot
{
RpcClientConnHandshake::RpcClientConnHandshake(RpcClientConn* )
//    : _state(eHandshakeState::SendCliPubKey), _conn(conn)
{
}

eCodes RpcClientConnHandshake::handshake(WsPacket *, std::unique_ptr<WsPacket> &)
{
    return eCodes::ST_Ok;
    // switch (_state)
    // {
    //     case eHandshakeState::SendCliPubKey:
    //     {
    //         outPkt.reset(new WsPacket());
    //     }
    //     break;

    //     case eHandshakeState::RecvSrvPubKeyAndSrvChallenge:
    //     {
    //         if (!inPkt)
    //         {
    //             PARROT_ASSERT(false);
    //         }
    //     }
    //     break;

    //     case eHandshakeState::SendCliChallengeAndSrvChallengeResult:
    //     {

    //     }
    //     break;

    //     case eHandshakeState::VerifySrvChallengeResult:
    //     {

    //     }
    //     break;

    //     default:
    //     {
    //         PARROT_ASSERT(false);
    //     }
    // }

    // return eCodes::ST_Ok;
}
}
