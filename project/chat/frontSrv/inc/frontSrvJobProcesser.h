#ifndef __PROJECT_CHAT_FRONTSRV_INC_FRONTSRVJOBPROCESSER_H__
#define __PROJECT_CHAT_FRONTSRV_INC_FRONTSRVJOBPROCESSER_H__

#include <list>
#include <memory>

#include "jobProcesser.h"
#include "threadJob.h"
#include "jobFactory.h"
#include "chatSession.h"
#include "rpcRequest.h"
#include "logicThread.h"

namespace chat
{
class JobHandler;

class FrontSrvJobProcesser : public parrot::JobProcesser
{
  public:
    FrontSrvJobProcesser();

  public:
    void setLogicThread(parrot::LogicThread* thread);

  public:
    void processJobs() override;

  protected:
    void loadJobs() override;

  protected:
    void processPacket(std::list<parrot::PacketJobParam<ChatSession>>& pktList);
    void processRpcResponse(
        std::list<parrot::RpcCliRspJobParam<ChatSession>>& pktList);
    void
    processUpdateSessionAck(std::list<std::shared_ptr<const ChatSession>>&);

  protected:
    parrot::RpcRequestContainer<ChatSession> _rpcReqContainer;
    parrot::PacketJobFactory<ChatSession> _pktJobFactory;

    parrot::UpdateSessionAckJobHdr<ChatSession> _updateSessAckJobHdr;
    parrot::PacketJobHdr<ChatSession> _packetJobHdr;
    parrot::RpcCliRspJobHdr<ChatSession> _rpcCliRspJobHdr;

    parrot::LogicThread* _logicThread;
};
}

#endif
