#ifndef __COMPONENT_SERVERGEAR_INC_RPCCLIENTCONNHANDSHAKE_H__
#define __COMPONENT_SERVERGEAR_INC_RPCCLIENTCONNHANDSHAKE_H__

#include <memory>
#include "wsPacket.h"
#include "codes.h"

namespace parrot
{
class RpcClientConn;

class RpcClientConnHandshake
{
    enum class eHandshakeState : uint8_t
    {
        SendCliPubKey,
        RecvSrvPubKeyAndSrvChallenge,
        SendCliChallengeAndSrvChallengeResult,
        VerifySrvChallengeResult
    };

  public:
    explicit RpcClientConnHandshake(RpcClientConn* conn);

  public:
    // Handshake with RpcServerConn using DH agorithm.
    eCodes handshake(WsPacket *inPkt, std::unique_ptr<WsPacket> &outPkt);

  private:
//    eHandshakeState _state;
//    RpcClientConn* _conn;
};
}

#endif
