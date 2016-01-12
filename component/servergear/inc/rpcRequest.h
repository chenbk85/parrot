#ifndef __COMPONENT_SERVERGEAR_INC_RPCREQUSET_H__
#define __COMPONENT_SERVERGEAR_INC_RPCREQUSET_H__

#include <string>
#include <memory>
#include <cstdint>
#include <sstream>

#include "wsDefinition.h"
#include "wsPacket.h"

#include "job.h"
#include "timeoutGuard.h"
#include "doubleLinkedListNode.h"
#include "jobFactory.h"

namespace parrot
{
class JobManager;

template <typename Sess>
class RpcRequest : public TimeoutGuard,
                   public DoubleLinkedListNode<RpcRequest<Sess>>,
                   public Job
{
  public:
    RpcRequest(const std::string& sid,
               std::shared_ptr<const Sess>& sess,
               std::unique_ptr<WsPacket>&& msg,
               JobManager* rspHandler);

  public:
    // Internal api.
    void setReqId(uint64_t reqId);
    // Internal api.
    const std::string& getDestSrvId() const;
    // Internal api.
    uint64_t getReqId() const;
    ePacketType getPacketType() const;
    std::unique_ptr<WsPacket>& getPacket();
    std::string toString();
    JobManager* getRspJobManager() const;
    std::shared_ptr<const Sess>& getSession();

  private:
    std::string _remoteSrvId;
    std::shared_ptr<const Sess> _session;
    std::unique_ptr<WsPacket> _packet;
    uint64_t _rpcReqId;
    JobManager* _rspJobMgr;
};

template <typename Sess>
RpcRequest<Sess>::RpcRequest(const std::string& sid,
                             std::shared_ptr<const Sess>& session,
                             std::unique_ptr<WsPacket>&& msg,
                             JobManager* rspJobMgr)
    : TimeoutGuard(),
      DoubleLinkedListNode<RpcRequest<Sess>>(),
      Job(JOB_RPC_CLI_REQ),
      _remoteSrvId(sid),
      _session(session),
      _packet(std::move(msg)),
      _rpcReqId(0),
      _rspJobMgr(rspJobMgr)
{
    // Inject session to sys json. So the remote knows who the identity of
    // requester.
    auto json = _packet->getSysJson();
    if (!json)
    {
        json = _packet->generateSysJson();
    }

    json->setValue("/session", _session->getJsonStr());

    // To avoid dynamic_cast from job.
    setDerivedPtr(this);
}

template <typename Sess> void RpcRequest<Sess>::setReqId(uint64_t reqId)
{
    _rpcReqId = reqId;
}

template <typename Sess> uint64_t RpcRequest<Sess>::getReqId() const
{
    return _rpcReqId;
}

template <typename Sess>
const std::string& RpcRequest<Sess>::getDestSrvId() const
{
    return _remoteSrvId;
}

template <typename Sess> ePacketType RpcRequest<Sess>::getPacketType() const
{
    return _packet->getPacketType();
}

template <typename Sess>
std::unique_ptr<WsPacket>& RpcRequest<Sess>::getPacket()
{
    return _packet;
}

template <typename Sess> JobManager* RpcRequest<Sess>::getRspJobManager() const
{
    return _rspJobMgr;
}

template <typename Sess>
std::shared_ptr<const Sess>& RpcRequest<Sess>::getSession()
{
    return _session;
}

template <typename Sess> std::string RpcRequest<Sess>::toString()
{
    std::ostringstream ostr;
    ostr << "ReqId is " << _rpcReqId << ". Packet Type is "
         << static_cast<uint16_t>(getPacketType()) << ". Session is "
         << _session->toString();
    return ostr.str();
}

template <typename Sess> using RpcRequestContainer = JobFactory<std::unique_ptr<Job>, Job>;
}

#endif
