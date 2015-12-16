#include <sstream>

#include "json.h"
#include "rpcRequest.h"

namespace parrot
{
RpcRequest::RpcRequest(const std::string& sid,
                       std::shared_ptr<Sess>&& session,
                       std::unique_ptr<WsPacket>&& msg)
    : _remoteSrvId(sid),
      _session(session),
      _packet(std::move(msg)),
      _rpcReqId(0)
{
    // Inject session to sys json. So the remote knows who the identity of
    // requester.
    auto json = _packet->getSysJson();
    if (!json)
    {
        json = _packet->generateSysJson();
    }

    json->setValue("/session", _session->getJsonStr());
}

void RpcRequest::onResponse(std::unique_ptr<WsPacket>&& pkt)
{
    std::list<SessionPktPair> pktList;
    pktList.emplace_back(std::move(_session), std::move(pkt));
    std::unique_ptr<PacketJob> job(new PacketJob());
    job->bind(std::move(pktList));
    _session->getRpcRspHdr(this)->addJob(job);
}

void RpcRequest::setReqId(uint64_t reqId)
{
    _rpcReqId = reqId;
}

uint64_t RpcRequest::getReqId() const
{
    return _rpcReqId;
}

const std::string& RpcRequest::getDestSrvId() const
{
    return _remoteSrvId;
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
