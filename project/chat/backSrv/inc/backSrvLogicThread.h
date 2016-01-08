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
#include "jobFactory.h"

namespace chat
{
struct BackSrvConfig;
class BackSrvMainThread;

class BackSrvLogicThread : public parrot::PoolThread, public parrot::JobHandler
{
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
    void handleJobs() override;

  protected:
    void handleRpcReq(std::list<parrot::RpcSrvReqJobParam>& pktList);
    void dispatchJobs();

  protected:
    BackSrvMainThread* _mainThread;
    parrot::RpcSrvReqJobHdr _reqPktJobHdr;
    std::unique_ptr<parrot::EventNotifier> _notifier;
    parrot::RpcSrvRspJobFactory _rpcSrvRspJobFactory;
    parrot::HdrJobListMap _hdrJobListMap;
    const BackSrvConfig* _config;
};
}
#endif
