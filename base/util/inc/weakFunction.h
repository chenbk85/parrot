#include <functional>
#include <memory>

namespace parrot
{
template<typename T1, typename... Ts>
class WeakFunction
{
  public:
    WeakFunction(std::weak_ptr<T1> &&wp,
                 std::function<void(Ts&&... params)> &&func):
        _func(std::move(func)),
        _weakPtr(std::move(wp))
    {
    }

    void fire(Ts&&... params)
    {
        if (_weakPtr.lock())
        {
            _func(std::forward<Ts>(params)...);
        }
    }

  private:
    std::function<void(Ts&&... params)> _func;
    std::weak_ptr<T1> _weakPtr;
};
}
