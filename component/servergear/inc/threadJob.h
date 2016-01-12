#ifndef __COMPONENT_SERVERGEAR_INC_THREADJOB_H__
#define __COMPONENT_SERVERGEAR_INC_THREADJOB_H__

#include <functional>
#include <memory>
#include <tuple>
#include <list>
#include <unordered_map>

#include "codes.h"
#include "json.h"
#include "seqGenHelper.h"
#include "job.h"
#include "rpcSession.h"

namespace parrot
{
class WsPacket;
class JobManager;
template <typename T, typename J> class JobFactory;

template <uint32_t JOBTYPE, typename... Ts> class ThreadJob : public Job
{
  public:
    ThreadJob() : Job(JOBTYPE), _args()
    {
        setDerivedPtr(this);
    }

    template <typename... RfTs> void bind(RfTs&&... args)
    {
        _args = std::forward_as_tuple(std::forward<RfTs>(args)...);
    }

    void call(const std::function<void(Ts&...)>& func)
    {
        apply(func, genSeq<sizeof...(Ts)>{});
    }

  private:
    template <std::size_t... Is>
    void apply(const std::function<void(Ts&...)>& func, seqIndex<Is...>)
    {
        func(std::get<Is>(_args)...);
    }

  private:
    std::tuple<Ts...> _args;
};

// Packet job & handler & factory.
//
//
template <typename Sess>
using PacketJobParam =
    std::pair<std::shared_ptr<const Sess>, std::unique_ptr<WsPacket>>;

template <typename Sess>
using PacketJob = ThreadJob<JOB_PACKET, std::list<PacketJobParam<Sess>>>;

template <typename Sess>
using PacketJobHdr = std::function<void(std::list<PacketJobParam<Sess>>&)>;

template <typename Sess>
using PacketJobFactory = JobFactory<PacketJobParam<Sess>, PacketJob<Sess>>;

// Rpc server request job & handler & factory.
// <RpcSession, ClientSession, WsPacket>
//
using RpcSrvReqJobParam = std::tuple<std::shared_ptr<RpcSession>,
                                     std::unique_ptr<Json>,
                                     std::unique_ptr<WsPacket>>;
using RpcSrvReqJob        = ThreadJob<JOB_RPC_SRV_REQ, std::list<RpcSrvReqJobParam>>;
using RpcSrvReqJobHdr     = std::function<void(std::list<RpcSrvReqJobParam>&)>;
using RpcSrvReqJobFactory = JobFactory<RpcSrvReqJobParam, RpcSrvReqJob>;

// Rpc server response job & handler & factory.
//
//
using RpcSrvRspJobParam =
    std::pair<std::shared_ptr<RpcSession>, std::unique_ptr<WsPacket>>;
using RpcSrvRspJob        = ThreadJob<JOB_RPC_SRV_RSP, std::list<RpcSrvRspJobParam>>;
using RpcSrvRspJobHdr     = std::function<void(std::list<RpcSrvRspJobParam>&)>;
using RpcSrvRspJobFactory = JobFactory<RpcSrvRspJobParam, RpcSrvRspJob>;

//
//
//
template <typename Sess>
using RpcCliRspJobParam =
    std::tuple<eCodes, std::shared_ptr<const Sess>, std::unique_ptr<WsPacket>>;

template <typename Sess>
using RpcCliRspJob =
    ThreadJob<JOB_RPC_CLI_RSP, std::list<RpcCliRspJobParam<Sess>>>;

template <typename Sess>
using RpcCliRspJobHdr =
    std::function<void(std::list<RpcCliRspJobParam<Sess>>&)>;

template <typename Sess>
using RpcCliRspJobFactory =
    JobFactory<RpcCliRspJobParam<Sess>, RpcCliRspJob<Sess>>;

//
//
//
template <typename Sess>
using UpdateSessionJobParam =
    std::pair<JobManager*, std::shared_ptr<const Sess>>;

template <typename Sess>
using UpdateSessionJob =
    ThreadJob<JOB_UPDATE_SESSION, std::list<UpdateSessionJobParam<Sess>>>;

template <typename Sess>
using UpdateSessionJobHdr =
    std::function<void(std::list<UpdateSessionJobParam<Sess>>&)>;

template <typename Sess>
using UpdateSessionJobFactory =
    JobFactory<UpdateSessionJobParam<Sess>, UpdateSessionJob<Sess>>;

//
//
//
template <typename Sess>
using UpdateSessionAckJob =
    ThreadJob<JOB_UPDATE_SESSION_ACK, std::list<std::shared_ptr<const Sess>>>;

template <typename Sess>
using UpdateSessionAckJobHdr =
    std::function<void(std::list<std::shared_ptr<const Sess>>&)>;

template <typename Sess>
using UpdateSessionAckJobFactory =
    JobFactory<std::shared_ptr<const Sess>, UpdateSessionAckJob<Sess>>;

//
//
//
using JobMgrListMap =
    std::unordered_map<JobManager*, std::list<std::unique_ptr<Job>>>;
}

#endif
