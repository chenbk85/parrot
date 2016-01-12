#include "frontSrvJobProcesser.h"
#include "frontSrvMainThread.h"
#include "logger.h"
#include "json.h"

namespace chat
{
FrontSrvJobProcesser::FrontSrvJobProcesser()
    : JobProcesser(),
      _rpcReqContainer(),
      _pktJobFactory(),
      _updateSessAckJobHdr(),
      _packetJobHdr(),
      _rpcCliRspJobHdr(),
      _logicThread(nullptr)

{
    using namespace std::placeholders;
    _packetJobHdr =
        std::bind(&FrontSrvJobProcesser::processPacket, this, _1);
    _updateSessAckJobHdr =
        std::bind(&FrontSrvJobProcesser::processUpdateSessionAck, this, _1);
    _rpcCliRspJobHdr =
        std::bind(&FrontSrvJobProcesser::processRpcResponse, this, _1);
}

void FrontSrvJobProcesser::setLogicThread(parrot::LogicPoolThread* thread)
{
    _logicThread = thread;
}

void FrontSrvJobProcesser::processPacket(
    std::list<parrot::PacketJobParam<ChatSession>>& pktList)
{
    for (auto& sp : pktList)
    {
        LOG_INFO("FrontSrvJobProcesser::processPacket: session is "
                 << (sp.first)->toString() << ".");

        auto pkt = (sp.second)->toResponsePkt();
        std::unique_ptr<parrot::Json> json(new parrot::Json());
        json->createRootObject();
        std::string s = "Hello world!";
        json->setValue("/s", s);
        pkt->setJson(std::move(json));

        // _clientPktMap[(sp.first)->getFrontJobHdr()].emplace_back(
        //     std::move(sp.first), std::move(pkt));

        // std::unique_ptr<parrot::RpcRequest<ChatSession>> rpcReq(
        //     new parrot::RpcRequest<ChatSession>(
        //         "backSrv001", sp.first,
        //         std::unique_ptr<parrot::WsPacket>(new parrot::WsPacket()),
        //         this));

        std::unique_ptr<parrot::Job> reqJob(new parrot::RpcRequest<ChatSession>(
            "backSrv001", sp.first,
            std::unique_ptr<parrot::WsPacket>(new parrot::WsPacket()),
            _logicThread));
        _rpcReqContainer.add(
            FrontSrvMainThread::getInstance()->getRpcCliThread(),
            std::move(reqJob));
    }

    // for (auto& kv : _clientPktMap)
    // {
    //     std::unique_ptr<parrot::PacketJob<ChatSession>> job(
    //         new parrot::PacketJob<ChatSession>());
    //     job->bind(std::move(kv.second));
    //     (kv.first)->addJob(std::move(job));
    //     kv.second.clear();
    // }
}

void FrontSrvJobProcesser::processRpcResponse(
    std::list<parrot::RpcCliRspJobParam<ChatSession>>& rspList)
{
    parrot::eCodes code;

    // <eCodes, ChatSession, WsPacket>
    for (auto& p : rspList)
    {
        code = std::get<0>(p);
        if (code != parrot::eCodes::ST_Ok)
        {
            std::system_error e(static_cast<int>(code),
                                parrot::ParrotCategory());
            LOG_WARN("FrontSrvJobProcesser::processRpcResponse: Rpc failed. "
                     "Err is "
                     << e.what() << ". Session is "
                     << std::get<1>(p)->toString() << ".");
            continue;
        }

        auto& session = std::get<1>(p);
        _pktJobFactory.add(session->getFrontJobMgr(),
                           parrot::PacketJobParam<ChatSession>(
                               std::move(session), std::move(std::get<2>(p))));
    }
}

void FrontSrvJobProcesser::processUpdateSessionAck(
    std::list<std::shared_ptr<const ChatSession>>&)
{
    // TODO: impl this function.
}

void FrontSrvJobProcesser::processJobs()
{
    for (auto& j : _jobList)
    {
        switch (j->getJobType())
        {
            case parrot::JOB_PACKET:
            {
                std::unique_ptr<parrot::PacketJob<ChatSession>> tj(
                    static_cast<parrot::PacketJob<ChatSession>*>(
                        (j.release())->getDerivedPtr()));
                tj->call(_packetJobHdr);
            }
            break;

            case parrot::JOB_RPC_CLI_RSP:
            {
                std::unique_ptr<parrot::RpcCliRspJob<ChatSession>> tj(
                    static_cast<parrot::RpcCliRspJob<ChatSession>*>(
                        (j.release())->getDerivedPtr()));
                tj->call(_rpcCliRspJobHdr);
            }
            break;

            case parrot::JOB_UPDATE_SESSION_ACK:
            {
                std::unique_ptr<parrot::UpdateSessionAckJob<ChatSession>> tj(
                    static_cast<parrot::UpdateSessionAckJob<ChatSession>*>(
                        (j.release())->getDerivedPtr()));
                tj->call(_updateSessAckJobHdr);
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

void FrontSrvJobProcesser::loadJobs()
{
    _pktJobFactory.loadJobs(_jobMgrListMap);
    _rpcReqContainer.loadJobsWithoutCreate(_jobMgrListMap);
}
}
