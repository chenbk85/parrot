#ifndef __BASE_SYS_INC_TIMEOUTMANAGER_H__
#define __BASE_SYS_INC_TIMEOUTMANAGER_H__

#include <cstdint>
#include <ctime>

#include "doubleLinkedList.h"
#include "timeoutGuard.h"
#include "timeoutHandler.h"

namespace parrot
{
template <typename T> class TimeoutManager
{
  public:
    TimeoutManager(TimeoutHandler<T>* handler, uint32_t timeoutSec)
        : _timeoutHandler(handler),
          _timeoutSeconds(static_cast<std::time_t>(timeoutSec)),
          _timeoutList()
    {
    }

    ~TimeoutManager()
    {
    }

    TimeoutManager(const TimeoutManager&) = delete;
    TimeoutManager& operator=(const TimeoutManager&) = delete;

  public:
    void add(T* tg, std::time_t now) noexcept
    {
        tg->setTime(now);
        _timeoutList.add(tg);
    }
    void remove(T* tg) noexcept
    {
        tg->setTime(static_cast<std::time_t>(0));
        _timeoutList.remove(tg);
    }

    void update(T* tg, std::time_t now) noexcept
    {
        remove(tg);
        add(tg, now);
    }

    void checkTimeout(std::time_t now) noexcept
    {
        T* guard = nullptr;
        while (true)
        {
            guard = _timeoutList.front();
            if (guard == nullptr)
            {
                break;
            }

            if (isTimeout(guard, now))
            {
                remove(guard);
                _timeoutHandler->onTimeout(guard);
            }
            else
            {
                break;
            }
        }
    }

  private:
    bool isTimeout(T* tg, std::time_t now) noexcept
    {
        if (tg->getTime() + _timeoutSeconds >= now)
        {
            return false;
        }

        return true;
    }

  private:
    TimeoutHandler<T>* _timeoutHandler;
    std::time_t _timeoutSeconds;
    DoubleLinkedList<T> _timeoutList;
};
}

#endif
