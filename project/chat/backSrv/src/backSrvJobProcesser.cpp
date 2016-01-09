#include "backSrvJobProcesser.h"
#include "backSrvMainThread.h"
#include "logger.h"
#include "json.h"

namespace chat
{
BackSrvJobProcesser::BackSrvJobProcesser()
    : JobProcesser(),
      _reqPktJobHdr(),
      _rpcSrvRspJobFactory(),
      _logicThread(nullptr)

{
    using namespace std::placeholders;
}

void BackSrvJobProcesser::setLogicThread(parrot::LogicThread* thread)
{
    _logicThread = thread;
}

void BackSrvJobProcesser::processRpcReq(
    std::list<parrot::RpcSrvReqJobParam>& pktList)
{
    // tuple<RpcSession, CliSessionJson, WsPacket>
    for (auto& p : pktList)
    {
        auto& pkt = std::get<2>(p);
        _rpcSrvRspJobFactory.add(
            BackSrvMainThread::getInstance()->getRpcSrvThread(),
            parrot::RpcSrvRspJobParam(std::move(std::get<0>(p)),
                                      std::move(pkt)));
    }
}

void BackSrvJobProcesser::processJobs()
{
    for (auto& j : _jobList)
    {
        switch (j->getJobType())
        {
            case parrot::JOB_RPC_SRV_REQ:
            {
                std::unique_ptr<parrot::RpcSrvReqJob> tj(
                    static_cast<parrot::RpcSrvReqJob*>(
                        (j.release())->getDerivedPtr()));
                tj->call(_reqPktJobHdr);
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

void BackSrvJobProcesser::loadJobs()
{
    _rpcSrvRspJobFactory.loadJobs(_hdrJobListMap);
}
}
