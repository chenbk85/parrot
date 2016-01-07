#ifndef __PROJECT_CHAT_BACKSRV_INC_BACKSRVLOGICTHREAD_H__
#define __PROJECT_CHAT_BACKSRV_INC_BACKSRVLOGICTHREAD_H__

#include <list>
#include <memory>
#include <mutex>
#include <functional>

#include "poolThread.h"
#include "wsPacket.h"
#include "job.h"
#include "json.h"
#include "rpcSession.h"
#include "threadJob.h"
#include "jobHandler.h"
#include "eventNotifier.h"

namespace chat
{
struct BackSrvConfig;
class BackSrvMainThread;

class BackSrvLogicThread : public parrot::PoolThread, public parrot::JobHandler
{
    using PktList = std::list<std::tuple<std::shared_ptr<parrot::RpcSession>,
                                         std::unique_ptr<parrot::Json>,
                                         std::unique_ptr<parrot::WsPacket>>>;

    using RpcRspList = std::list<std::pair<std::shared_ptr<parrot::RpcSession>,
                                           std::unique_ptr<parrot::WsPacket>>>;

  public:
    BackSrvLogicThread();

  public:
    void setConfig(const BackSrvConfig* cfg);
    void setMainThread(BackSrvMainThread* mainThread);

  public:
    void stop() override;
    void afterAddJob() override;

  protected:
    void beforeStart() override;
    void run() override;

  protected:
    void handleJob() override;

  protected:
    void handleRpcReq(PktList& pktList);

  protected:
    BackSrvMainThread* _mainThread;
    parrot::RpcSrvReqJobHdr _reqPktJobHdr;
    std::unique_ptr<parrot::EventNotifier> _notifier;
    RpcRspList _rpcRspList;
    const BackSrvConfig* _config;
};
}
#endif
