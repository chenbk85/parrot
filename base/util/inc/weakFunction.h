#include <functional>
#include <memory>
#include <iostream>

namespace parrot
{
template <typename T1, typename... Ts> class WeakFunction
{
  public:
    WeakFunction(std::weak_ptr<T1>&& wp,
                 std::function<void(const Ts&... params)>&& func)
        : _func(std::move(func)), _cb(), _weakPtr(std::move(wp))
    {
    }

    template<typename... RfTs>
    void bind(RfTs&&... params)
    {
        _cb = std::bind(_func, std::forward<RfTs>(params)...);
    }

    void fire()
    {
        if (_weakPtr.lock())
        {
            _cb();
        }        
    }

    /* void fire(Ts... params) */
    /* { */
    /*     if (_weakPtr.lock()) */
    /*     { */
    /*         _func(std::forward<Ts>(params)...); */
    /*     } */
    /* } */

  private:
    std::function<void(const Ts&... params)> _func;
    std::function<void()> _cb;
    std::weak_ptr<T1> _weakPtr;
};
}
