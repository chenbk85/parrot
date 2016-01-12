#ifndef __COMPONENT_SERVERGEAR_INC_RPCSERVERJOBPROCESSER_H__
#define __COMPONENT_SERVERGEAR_INC_RPCSERVERJOBPROCESSER_H__

#include <memory>
#include <tuple>
#include <functional>

#include "jobProcesser.h"
#include "threadJob.h"

namespace parrot
{
class JobManager;
class RpcServerThread;

class RpcServerJobProcesser : public JobProcesser
{
  public:
    explicit RpcServerJobProcesser(RpcServerThread* thread);

  public:
    void createRpcReqJob(JobManager* mgr, RpcSrvReqJobParam&& jobParam);

  public:
    void processJobs() override;

  protected:
    void loadJobs() override;

  protected:
    void processRpcRsp(std::list<RpcSrvRspJobParam>& jobParamList);

  protected:
    RpcSrvReqJobFactory _rpcSrvReqJobFactory;
    RpcSrvRspJobHdr _rpcRspJobHdr;
    RpcServerThread* _rpcSrvThread;
};
}

#endif
