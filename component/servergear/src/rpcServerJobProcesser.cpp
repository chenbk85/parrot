#include "sysDefinitions.h"
#include "rpcServerThread.h"
#include "rpcServerJobProcesser.h"
#include "macroFuncs.h"
#include "logger.h"

namespace parrot
{
RpcServerJobProcesser::RpcServerJobProcesser(RpcServerThread* thread)
    : _rpcSrvReqJobFactory(), _rpcRspJobHdr(), _rpcSrvThread(thread)
{
    using namespace std::placeholders;
    _rpcRspJobHdr = std::bind(&RpcServerJobProcesser::processRpcRsp, this, _1);
}

void RpcServerJobProcesser::createRpcReqJob(JobHandler* hdr,
                                            RpcSrvReqJobParam&& jobParam)
{
    _rpcSrvReqJobFactory.add(hdr, std::move(jobParam));
}

void RpcServerJobProcesser::processRpcRsp(
    std::list<RpcSrvRspJobParam>& jobParamList)
{
    std::unordered_map<std::string, RpcServerConn*>::iterator it;
    auto& registeredConnMap = _rpcSrvThread->_registeredConnMap;
    for (auto& s : jobParamList)
    {
        it = registeredConnMap.find((s.first)->getRemoteSid());
        if (it == registeredConnMap.end())
        {
            LOG_WARN("RpcServerJobProcesser::processRpcRsp: Failed to find "
                     "remote sid "
                     << (s.first)->getRemoteSid() << ". Packet is discarded. "
                     << "SysJson is " << (s.second)->getSysJson()->toString()
                     << ".");
            continue;
        }

        it->second->sendPacket(s.second);
        if (it->second->canSwitchToSend())
        {
            it->second->setNextAction(eIoAction::Write);
            _rpcSrvThread->_notifier->updateEventAction(it->second);
        }

        LOG_DEBUG(
            "RpcServerJobProcesser::processRpcRsp: Sending packet. SysJson is "
            << (s.second)->getSysJson()->toString() << ".");
    }
}

void RpcServerJobProcesser::processJobs()
{
    for (auto& j : _jobList)
    {
        switch (j->getJobType())
        {
            case JOB_RPC_SRV_RSP:
            {
                std::unique_ptr<RpcSrvRspJob> tj(
                    static_cast<RpcSrvRspJob*>((j.release())->getDerivedPtr()));
                tj->call(_rpcRspJobHdr);
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

void RpcServerJobProcesser::loadJobs()
{
    _rpcSrvReqJobFactory.loadJobs(_hdrJobListMap);
}
}
