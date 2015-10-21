#ifndef __BASE_UTIL_INC_WEAKFUNCTION_H__
#define __BASE_UTIL_INC_WEAKFUNCTION_H__

#include <functional>
#include <memory>
#include <tuple>

#include "seqGenHelper.h"

/**
 * Before use this template, please ref the following link:
 *   http://goo.gl/Fskd9E
 */
namespace parrot
{
template <typename T, typename... Ts> class WeakFunction
{
  public:
    WeakFunction(std::weak_ptr<T>&& wp,
                 std::function<void(const Ts&... params)>&& func)
        : _cb(std::move(func)), _args(), _owner(std::move(wp))
    {
    }

    template <typename... RfTs> void bind(RfTs&&... args)
    {
        _args = std::forward_as_tuple(std::forward<RfTs>(args)...);
    }

    void fire()
    {
        auto sp = _owner.lock();
        if (sp)
        {
            apply(genSeq<sizeof...(Ts)>{});
        }
    }

  private:
    template<std::size_t... Is> void apply(seqIndex<Is...>)
    {
        _cb(std::get<Is>(_args)...);
    }

  private:
    std::function<void(const Ts&... params)> _cb;
    std::tuple<Ts...> _args;
    std::weak_ptr<T> _owner;
};
}

#endif
