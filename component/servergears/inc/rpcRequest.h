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
template <typename Sess>
class RpcRequest : public TimeoutGuard,
                   public DoubleLinkedListNode<RpcRequest<Sess>>
{
  public:
    RpcRequest(const std::string& sid,
               std::shared_ptr<Sess>&& sess,
               std::unique_ptr<WsPacket>&& msg,
               JobHandler* rspHandler);

  public:
    void onResponse(std::unique_ptr<WsPacket>&& rsp);
    void setReqId(uint64_t reqId);
    const std::string& getDestSrvId() const;
    uint64_t getReqId() const;
    ePacketType getPacketType() const;
    std::unique_ptr<WsPacket>& getPacket();
    std::string toString();

  private:
    std::string _remoteSrvId;
    std::shared_ptr<Sess> _session;
    std::unique_ptr<WsPacket> _packet;
    uint64_t _rpcReqId;
    JobHandler* _rspHandler;
};

template <typename Sess>
RpcRequest<Sess>::RpcRequest(const std::string& sid,
                             std::shared_ptr<Sess>&& session,
                             std::unique_ptr<WsPacket>&& msg,
                             JobHandler* rspHandler)
    : _remoteSrvId(sid),
      _session(session),
      _packet(std::move(msg)),
      _rpcReqId(0),
      _rspHandler(rspHandler)
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

template <typename Sess>
void RpcRequest::onResponse(std::unique_ptr<WsPacket>&& pkt)
{
    std::list<SessionPktPair<Sess>> pktList;
    pktList.emplace_back(std::move(_session), std::move(pkt));
    std::unique_ptr<PacketJob<Sess>> job(new PacketJob<Sess>());
    job->bind(std::move(pktList));
    _session->getRpcRspHdr(this)->addJob(job);
}

template <typename Sess> void RpcRequest::setReqId(uint64_t reqId)
{
    _rpcReqId = reqId;
}

template <typename Sess> uint64_t RpcRequest::getReqId() const
{
    return _rpcReqId;
}

template <typename Sess> const std::string& RpcRequest::getDestSrvId() const
{
    return _remoteSrvId;
}

template <typename Sess> ePacketType RpcRequest::getPacketType() const
{
    return _packet->getPacketType();
}

template <typename Sess> std::unique_ptr<WsPacket>& RpcRequest::getPacket()
{
    return _packet;
}

template <typename Sess> std::string RpcRequest::toString()
{
    std::ostringstream ostr;
    ostr << "ReqId is " << _rpcReqId << ". Packet Type is "
         << static_cast<uint16_t>(getPacketType()) << ". Requester is "
         << _requester->toString();
    return std::move(ostr.str());
}
}

#endif
