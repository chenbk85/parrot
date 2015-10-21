#ifndef __BASE_UTIL_INC_SHAREDFUNCTION_H__
#define __BASE_UTIL_INC_SHAREDFUNCTION_H__

#include <functional>
#include <memory>

#include "seqGenHelper.h"

/**
 * Before use this template, please ref the following link:
 *   http://goo.gl/Fskd9E
 */
namespace parrot
{
template <typename T, typename... Ts> class SharedFunction
{
  public:
    SharedFunction(std::shared_ptr<T>& wp,
                   std::function<void(Ts&... params)>&& func)
        : _cb(std::move(func)), _args(), _owner(wp)
    {
    }

    template <typename... RfTs> void bind(RfTs&&... args)
    {
        _args = std::forward_as_tuple(std::forward<RfTs>(args)...);
    }

    void fire()
    {
        apply(genSeq<sizeof...(Ts)>{});        
    }

  private:
    template<std::size_t... Is> void apply(seqIndex<Is...>)
    {
        _cb(std::get<Is>(_args)...);
    }
    
  private:
    std::function<void(Ts&... params)> _cb;
    std::tuple<Ts...> _args;    
    std::shared_ptr<T> _owner;
};
}

#endif
