#ifndef __COMPONENT_SERVERGEAR_INC_THREADJOB_H__
#define __COMPONENT_SERVERGEAR_INC_THREADJOB_H__

#include <functional>
#include <memory>
#include <tuple>

#include "seqGenHelper.h"
#include "job.h"

namespace parrot
{
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

class WsPacket;

//
//
//
template <typename Sess>
using SessionPktPair =
    std::pair<std::shared_ptr<const Sess>, std::unique_ptr<WsPacket>>;

//
//
//
template <typename Sess>
using PacketJob = ThreadJob<JOB_PACKET, std::list<SessionPktPair<Sess>>>;

//
//
//
template <typename Sess>
using PacketJobHdr = std::function<void(std::list<SessionPktPair<Sess>>&)>;

//
//
//
template <typename Sess>
using RspBindJob =
    ThreadJob<JOB_RSP_BIND, std::list<std::shared_ptr<const Sess>>>;

//
//
//
template <typename Sess>
using RspBindJobHdr =
    std::function<void(std::list<std::shared_ptr<const Sess>>&)>;

//
//
//
template <typename Sess>
using UpdateSessionJob =
    ThreadJob<JOB_UPDATE_SESSION, std::shared_ptr<const Sess>>;

//
//
//
template <typename Sess>
using UpdateSessionJobHdr = std::function<void(std::shared_ptr<const Sess>&)>;
}

#endif
