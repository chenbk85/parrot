#include <functional>
#include <memory>

namespace parrot
{
template <typename T, typename... Ts> class SharedFunction
{
  public:
    SharedFunction(std::shared_ptr<T>& wp,
                   std::function<void(Ts&... params)>&& func)
        : _cb(std::move(func)), _cbWithArgs(), _owner(wp)
    {
    }

    template <typename... RfTs> void bind(RfTs&&... args)
    {
        _cbWithArgs = std::bind(_cb, std::forward<RfTs>(args)...);
    }

    void fire()
    {
        _cbWithArgs();
    }

  private:
    std::function<void(Ts&... params)> _cb;
    std::function<void()> _cbWithArgs;
    std::shared_ptr<T> _owner;
};
}
