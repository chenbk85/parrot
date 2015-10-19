#include <functional>
#include <memory>

/**
 * Be sure to make the parameters of callback function moveable, or
 * the complier will copy the arguments for 3 times. This is not
 * noticeable and very bad. If the parameters are moveable, the
 * complier will copy one time when assign the bind result. So, if
 * the parameter takes too much memory, pass a smart pointer will
 * be a good choice.
 *
 * Please ref the following link:
 *   http://stackoverflow.com/questions/33198974/too-many-copies-when-binding-variadic-template-arguments
 */
namespace parrot
{
template <typename T, typename... Ts> class WeakFunction
{
  public:
    WeakFunction(std::weak_ptr<T>&& wp,
                 std::function<void(const Ts&... params)>&& func)
        : _cb(std::move(func)), _cbWithArgs(), _owner(std::move(wp))
    {
    }

    template <typename... RfTs> void bind(RfTs&&... args)
    {
        _cbWithArgs = std::bind(_cb, std::forward<RfTs>(args)...);
    }

    void fire()
    {
        if (_owner.lock())
        {
            _cbWithArgs();
        }
    }

  private:
    std::function<void(const Ts&... params)> _cb;
    std::function<void()> _cbWithArgs;
    std::weak_ptr<T> _owner;
};
}
