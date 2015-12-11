#include <sstream>

#include "json.h"
#include "rpcRequest.h"

namespace parrot
{
RpcRequest::RpcRequest(std::unique_ptr<RpcRequester>&& requester,
                       std::unique_ptr<WsPacket>&& msg)
    : _requester(std::move(requester)), _packet(std::move(msg)), _rpcReqId(0)
{
    _packet->getSysJson()->setValue("/session", _requester->_session);
}

void RpcRequest::onResponse(std::unique_ptr<WsPacket>&&)
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

ePacketType RpcRequest::getPacketType() const
{
    return _packet->getPacketType();
}

std::unique_ptr<WsPacket>& RpcRequest::getPacket()
{
    return _packet;
}

std::string RpcRequest::toString()
{
    std::ostringstream ostr;
    ostr << "ReqId is " << _rpcReqId << ". Packet Type is "
         << static_cast<uint16_t>(getPacketType()) << ". Requester is "
         << _requester->toString();
    return std::move(ostr.str());
}
}
