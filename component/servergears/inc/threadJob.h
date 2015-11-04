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

struct Session;
class WsPacket;
class FrontThread;

using SessionPktPair =
    std::pair<std::shared_ptr<const Session>, std::unique_ptr<WsPacket>>;

using PacketJob    = ThreadJob<eJobType::Packet, std::list<SessionPktPair>>;
using PacketJobHdr = std::function<void(std::list<SessionPktPair>&)>;

using ReqBindJob =
    ThreadJob<eJobType::ReqBind, FrontThread*, std::list<SessionPktPair>>;
using ReqBindJobHdr =
    std::function<void(FrontThread*, std::list<SessionPktPair>&)>;

using RspBindJob =
    ThreadJob<eJobType::RspBind, std::list<std::shared_ptr<const Session>>>;
using RspBindJobHdr =
    std::function<void(std::list<std::shared_ptr<const Session>>&)>;

using UpdateSessionJob =
    ThreadJob<eJobType::UpdateSession, std::shared_ptr<const Session>>;
using UpdateSessionJobHdr =
    std::function<void(std::shared_ptr<const Session>&)>;
}

#endif
