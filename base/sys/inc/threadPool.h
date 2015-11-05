#ifndef __BASE_SYS_INC_THREADPOOL_H__
#define __BASE_SYS_INC_THREADPOOL_H__

#include <cstdint>
#include <vector>
#include <thread>
#include <chrono>
#include <memory>

#include "macroFuncs.h"

namespace parrot
{
template <typename ThreadClass> class ThreadPool
{
  public:
    ThreadPool() : _count(0), _threadVec()
    {
    }
    
    explicit ThreadPool(uint32_t count) : _count(count), _threadVec()
    {
    }

    ~ThreadPool()
    {
        PARROT_ASSERT(_count == 0);
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

  public:
    void setCount(uint32_t count)
    {
        _count = count;
    }

    void getCount() const
    {
        return _count;
    }
    
    void create()
    {
        PARROT_ASSERT(_count > 0);
        for (auto i = 0u; i != _count; ++i)
        {
            _threadVec.emplace_back(new ThreadClass());
            _threadVec[i]->start();

            while (!_threadVec[i]->isSleeping())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
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
        for (auto& t : _threadVec)
        {
            t->wakeUp();
        }
    }

    void destroy()
    {
        for (auto& t : _threadVec)
        {
            t->stop();
        }

        _count = 0;
    }

  private:
    uint32_t _count;
    std::vector<std::unique_ptr<ThreadClass>> _threadVec;
};
}
#endif
