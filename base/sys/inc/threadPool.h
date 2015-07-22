#ifndef __BASE_SYS_INC_THREADPOOL_H__
#define __BASE_SYS_INC_THREADBASE_H__

#include <cstdint>
#include <vector>
#include <memory>

#include "macroFuncs.h"

namespace parrot
{
    template<typename ThreadClass>
    class ThreadPool
    {
      public:
        ThreadPool(uint32_t count):
            _count(count),
            _threadVec()
        {
        }

        ~ThreadPool()
        {
            destroy();
        }

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool &operator=(const ThreadPool&) = delete;

      public:
        void create()
        {
#ifdef DEBUG
            ASSERT(_count > 0);
#endif
            for (auto i = 0u; i != _count; ++i)
            {
                _threadVec.embrace_back(
                    std::unique_ptr<ThreadClass>(new ThreadClass()));
                _threadVec[i].start();
            }
        }

        ThreadClass* getThreadByIdx(uint32_t idx)
        {
            return _threadVec[idx].get();
        }

        std::vector<std::unique_ptr<ThreadClass>>& getThreadPoolVec() noexcept
        {
            return _threadVec;
        }

        void start()
        {
            for (auto &t : _threadVec)
            {
                t->wakeUp();
            }
        }

        void destroy()
        {
            for (auto &t : _threadVec)
            {
                t->stop();
            }
            _threadVec.clear();
        }

      private:
        uint32_t                                  _count;
        std::vector<std::unique_ptr<ThreadClass>> _threadVec; 
    };
}
#endif
