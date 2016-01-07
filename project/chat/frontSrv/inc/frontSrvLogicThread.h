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
    using RpcRspList = std::list<std::tuple<parrot::eCodes,
                                            std::shared_ptr<ChatSession>,
                                            std::unique_ptr<parrot::WsPacket>>>;

    using ClientPktMap =
        std::unordered_map<parrot::JobHandler*,
                           std::list<parrot::SessionPktPair<ChatSession>>>;

    using RpcReqList =
        std::list<std::unique_ptr<parrot::Job>>;

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
    void handleJob() override;

  protected:
    void handlePacket(std::list<parrot::SessionPktPair<ChatSession>>& pktList);
    void handleRpcResponse(RpcRspList& pktList);

  protected:
    FrontSrvMainThread* _mainThread;
    parrot::PacketJobHdr<ChatSession> _packetJobHdr;
    std::unique_ptr<parrot::EventNotifier> _notifier;
    RpcReqList _rpcReqList;
    ClientPktMap _clientPktMap;
    const FrontSrvConfig* _config;
};
}
#endif
