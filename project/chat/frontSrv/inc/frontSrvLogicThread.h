#ifndef __PROJECT_CHAT_FRONTSRV_INC_FRONTSRVLOGICTHREAD_H__
#define __PROJECT_CHAT_FRONTSRV_INC_FRONTSRVLOGICTHREAD_H__

#include <list>
#include <memory>
#include <mutex>
#include <tuple>
#include <unordered_map>

#include "codes.h"
#include "poolThread.h"
#include "wsPacket.h"
#include "job.h"
#include "threadJob.h"
#include "jobHandler.h"
#include "chatSession.h"
#include "rpcRequest.h"
#include "eventNotifier.h"

namespace chat
{
struct FrontSrvConfig;
class FrontSrvMainThread;

class FrontSrvLogicThread : public parrot::PoolThread, public parrot::JobHandler
{
  public:
    FrontSrvLogicThread();

  public:
    void setConfig(const FrontSrvConfig* cfg);
    void setMainThread(FrontSrvMainThread* mainThread);

  public:
    void stop() override;
    void afterAddJob() override;

  protected:
    void beforeStart() override;
    void run() override;

  protected:
    void handleJobs() override;

  protected:
    void handlePacket(std::list<parrot::PacketJobParam<ChatSession>>& pktList);
    void handleRpcResponse(
        std::list<parrot::RpcCliRspJobParam<ChatSession>>& pktList);
    void handleUpdateSessionAck(std::list<std::shared_ptr<const ChatSession>>&);
    void dispatchJobs();

  protected:
    FrontSrvMainThread* _mainThread;
    std::unique_ptr<parrot::EventNotifier> _notifier;

    parrot::RpcRequestContainer<ChatSession> _rpcReqContainer;
    parrot::PacketJobFactory<ChatSession> _pktJobFactory;
    parrot::HdrJobListMap _hdrJobListMap;

    parrot::UpdateSessionAckJobHdr<ChatSession> _updateSessAckJobHdr;
    parrot::PacketJobHdr<ChatSession> _packetJobHdr;
    parrot::RpcCliRspJobHdr<ChatSession> _rpcCliRspJobHdr;

    const FrontSrvConfig* _config;
};
}
#endif
