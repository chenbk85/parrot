#ifndef __BASE_SYS_INC_TIMEOUTMANAGER_H__
#define __BASE_SYS_INC_TIMEOUTMANAGER_H__

#include <list>
#include <chrono>

namespace parrot
{
    class TimeoutManager
    {
      public:
        TimeoutManager();
        ~TimeoutManager();
        TimeoutManager(const TimeoutManager&) = delete;
        TimeoutManager& operator=(const TimeoutManager&) = delete;

      public:
        void add(const TimeoutGuard *tg, const std::chrono::time_point &tp);
        void checkTimeout();
        void onTimeout();

      private:
        std::List<const TimeoutGuard*> _timeoutList;
    };
}

#endif
