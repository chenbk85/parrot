#ifndef __COMPONENT_SERVERGEAR_INC_RPCREQUSET_H__
#define __COMPONENT_SERVERGEAR_INC_RPCREQUSET_H__

#include <string>
#include <memory>
#include <cstdint>

#include "wsDefinition.h"
#include "wsPacket.h"

#include "rpcRequester.h"
#include "timeoutGuard.h"
#include "doubleLinkedListNode.h"

namespace parrot
{
class RpcRequest : public TimeoutGuard, public DoubleLinkedListNode<RpcRequest>
{
  public:
    RpcRequest(std::unique_ptr<RpcRequester>&& requester,
               std::unique_ptr<WsPacket>&& msg);
    RpcRequest() = default;

  public:
    void onResponse(std::unique_ptr<WsPacket>&& rsp);
    void setReqId(uint64_t reqId);
    uint64_t getReqId() const;
    ePacketType getPacketType() const;
    std::unique_ptr<WsPacket>& getPacket();
    std::string toString();

  private:
    std::unique_ptr<RpcRequester> _requester;
    std::unique_ptr<WsPacket> _packet;
    uint64_t _rpcReqId;
};
}

#endif
