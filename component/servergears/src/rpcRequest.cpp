#include <sstream>

#include "rpcRequest.h"

namespace parrot
{
RpcRequest::RpcRequest(std::unique_ptr<RpcRequester>&& requester,
                       std::unique_ptr<WsPacket>&& msg):
    _requester(std::move(requester)),
    _packet(std::move(msg)),
    _rpcReqId(0)
{
}

void RpcRequest::onResponse(std::unique_ptr<WsPacket>&& )
{
}

void RpcRequest::setReqId(uint64_t reqId)
{
    _rpcReqId = reqId;
}

uint64_t RpcRequest::getReqId() const
{
    return _rpcReqId;
}

std::string RpcRequest::toString()
{
    std::ostringstream ostr;
    ostr << "ReqId is " << _rpcReqId << ". Requester is "
         << _requester->toString();
    return std::move(ostr.str());
}
}
