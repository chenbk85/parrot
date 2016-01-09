#ifndef __PROJECT_CHAT_BACKSRV_INC_BACKSRVJOBPROCESSER_H__
#define __PROJECT_CHAT_BACKSRV_INC_BACKSRVJOBPROCESSER_H__

#include <list>
#include <memory>

#include "jobProcesser.h"
#include "threadJob.h"
#include "jobFactory.h"
#include "chatSession.h"
#include "logicThread.h"

namespace chat
{
class JobHandler;

class BackSrvJobProcesser : public parrot::JobProcesser
{
  public:
    BackSrvJobProcesser();

  public:
    void setLogicThread(parrot::LogicThread* thread);

  public:
    void processJobs() override;

  protected:
    void loadJobs() override;

  protected:
    void processRpcReq(std::list<parrot::RpcSrvReqJobParam>& pktList);

  protected:
    parrot::RpcSrvReqJobHdr _reqPktJobHdr;
    parrot::RpcSrvRspJobFactory _rpcSrvRspJobFactory;
    parrot::LogicThread* _logicThread;
};
}

#endif
