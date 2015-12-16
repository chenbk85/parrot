#ifndef __COMPONENT_SERVERGEAR_INC_THREADJOB_H__
#define __COMPONENT_SERVERGEAR_INC_THREADJOB_H__

#include <functional>
#include <memory>
#include <tuple>

#include "seqGenHelper.h"
#include "job.h"

namespace parrot
{
template <eJobType JOBTYPE, typename... Ts> class ThreadJob : public Job
{
  public:
    ThreadJob() : Job(JOBTYPE), _args()
    {
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
class FrontThread;

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
using PacketJob = ThreadJob<eJobType::Packet, std::list<SessionPktPair<Sess>>>;

//
//
//
template <typename Sess>
using PacketJobHdr = std::function<void(std::list<SessionPktPair<Sess>>&)>;

//
//
//
template <typename Sess>
using ReqBindJob = ThreadJob<eJobType::ReqBind,
                             FrontThread<Sess>*,
                             std::list<SessionPktPair<Sess>>>;

//
//
//
template <typename Sess>
using ReqBindJobHdr =
    std::function<void(FrontThread<Sess>*, std::list<SessionPktPair<Sess>>&)>;

//
//
//
template <typename Sess>
using RspBindJob =
    ThreadJob<eJobType::RspBind, std::list<std::shared_ptr<const Sess>>>;

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
    ThreadJob<eJobType::UpdateSession, std::shared_ptr<const Sess>>;

//
//
//
template <typename Sess>
using UpdateSessionJobHdr = std::function<void(std::shared_ptr<const Sess>&)>;
}

#endif
