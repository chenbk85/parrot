#ifndef __COMPONENT_SERVERGEAR_INC_RPCCLIENTJOBPROCESSER_H__
#define __COMPONENT_SERVERGEAR_INC_RPCCLIENTJOBPROCESSER_H__

#include <memory>
#include <tuple>
#include <functional>

#include "jobProcesser.h"
#include "threadJob.h"
#include "rpcClientThread.h"

namespace parrot
{
class JobManager;
template <typename Sess> class RpcClientThread;

template <typename Sess> class RpcClientJobProcesser : public JobProcesser
{
  public:
    explicit RpcClientJobProcesser(RpcClientThread<Sess>*);

  public:
    void createRpcRspJob(JobManager* hdr, RpcCliRspJobParam<Sess>&& jobParam);

  public:
    void processJobs() override;

  protected:
    void loadJobs() override;

  protected:
    void handleRpcCliReq(std::unique_ptr<RpcRequest<Sess>>& req);

  protected:
    RpcCliRspJobFactory<Sess> _rpcCliRspJobFactory;

    RpcClientThread<Sess>* _rpcCliThread;
};

template <typename Sess>
RpcClientJobProcesser<Sess>::RpcClientJobProcesser(
    RpcClientThread<Sess>* thread)
    : JobProcesser(), _rpcCliRspJobFactory(), _rpcCliThread(thread)
{
    using namespace std::placeholders;
}

template <typename Sess>
void RpcClientJobProcesser<Sess>::createRpcRspJob(
    JobManager* mgr, RpcCliRspJobParam<Sess>&& jobParam)
{
    _rpcCliRspJobFactory.add(mgr, std::move(jobParam));
}

template <typename Sess>
void RpcClientJobProcesser<Sess>::handleRpcCliReq(
    std::unique_ptr<RpcRequest<Sess>>& req)
{
    auto& connMap = _rpcCliThread->_connMap;
    auto mit      = connMap.end();

    mit = connMap.find(req->getDestSrvId());
    if (mit == connMap.end())
    {
        LOG_WARN(
            "RpcClientJobProcesser::handleRpcCliReq: Failed to find rpc server "
            << req->getDestSrvId() << ".");

        createRpcRspJob(
            req->getRspJobManager(),
            RpcCliRspJobParam<Sess>(eCodes::ERR_RemoteNotConnected,
                                    std::move(req->getSession()),
                                    std::unique_ptr<WsPacket>(new WsPacket())));
        return;
    }

    mit->second->addJob(req);
    if (mit->second->canSwitchToSend())
    {
        mit->second->setNextAction(eIoAction::Write);
        _rpcCliThread->_notifier->updateEventAction(mit->second.get());
    }
}

template <typename Sess> void RpcClientJobProcesser<Sess>::processJobs()
{
    for (auto& j : _jobList)
    {
        switch (j->getJobType())
        {
            case JOB_RPC_CLI_REQ:
            {
                std::unique_ptr<RpcRequest<Sess>> req(
                    static_cast<RpcRequest<Sess>*>(
                        (j.release())->getDerivedPtr()));
                handleRpcCliReq(req);
            }
            break;

            default:
            {
                PARROT_ASSERT(false);
            }
            break;
        }
    }
}

template <typename Sess> void RpcClientJobProcesser<Sess>::loadJobs()
{
    _rpcCliRspJobFactory.loadJobs(_jobMgrListMap);
}
}

#endif
