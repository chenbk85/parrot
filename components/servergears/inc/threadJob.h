#ifndef __COMPONENT_SERVERGEAR_INC_THREADJOB_H__
#define __COMPONENT_SERVERGEAR_INC_THREADJOB_H__

#include <functional>
#include <memory>
#include <tuple>

#include "seqGenHelper.h"
#include "job.h"

namespace parrot
{
template <typename... Ts> class ThreadJob : public Job
{
  public:
    ThreadJob(JobType jobType) : Job(jobType), _args()
    {
    }

    template <typename... RfTs> void bind(RfTs&&... args)
    {
        _args = std::forward_as_tuple(std::forward<RfTs>(args)...);
    }

    void call(const std::function<void(Ts&...)> &func)
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
}

#endif
