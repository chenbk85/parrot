#ifndef __COMPONENT_SERVERGEAR_INC_RPCSERVERCONNHANDSHAKE_H__
#define __COMPONENT_SERVERGEAR_INC_RPCSERVERCONNHANDSHAKE_H__

#include <memory>
#include "wsPacket.h"
#include "codes.h"

namespace parrot
{
class RpcServerConn;

class RpcServerConnHandshake
{
    enum class eHandshakeState : uint8_t
    {
        RecvCliPubKey,
        SendSrvPubKeyAndSrvChallenge,
        RecvCliChallengeAndSendChallengeResult,
        VerifyCliChallengeResult
    };

  public:
    explicit RpcServerConnHandshake(RpcServerConn* conn);

  public:
    // Handshake with RpcClientConn using DH agorithm.
    eCodes handshake(WsPacket* inPkt, std::unique_ptr<WsPacket>& outPkt);

  private:
    eHandshakeState _state;
    RpcServerConn* _conn;
};
}

#endif
