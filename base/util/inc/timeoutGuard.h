#ifndef __BASE_SYS_INC_TIMEOUTGUARD_H__
#define __BASE_SYS_INC_TIMEOUTGUARD_H__

#include <ctime>

namespace parrot
{
class TimeoutGuard
{
  public:
    TimeoutGuard() noexcept : _lastActiveTime(0)
    {
    }
    virtual ~TimeoutGuard()
    {
    }

    TimeoutGuard(const TimeoutGuard&) = delete;
    TimeoutGuard& operator=(const TimeoutGuard&) = delete;

  public:
    inline void setTime(std::time_t now) noexcept
    {
        _lastActiveTime = now;
    }

    inline std::time_t getTime() const noexcept
    {
        return _lastActiveTime;
    }

  private:
    std::time_t _lastActiveTime;
};
}

#endif
