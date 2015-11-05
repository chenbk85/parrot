#include <chrono>

#include "threadBase.h"
#include "macroFuncs.h"

namespace parrot
{

ThreadBase::ThreadBase()
    : _threadPtr(),
      _state(ThreadState::Init),
      _sleeping(false),
      _lock(),
      _condVar()
{
}

ThreadBase::~ThreadBase()
{
    if (_threadPtr.get() != nullptr)
    {
        PARROT_ASSERT(0);
    }
}

void ThreadBase::start()
{
    beforeStart();
    _state = ThreadState::Started;
    _threadPtr = std::unique_ptr<std::thread>(
        new std::thread(&ThreadBase::entryFunc, this));
}

void ThreadBase::entryFunc()
{
    beforeRun();
    run();
    _state = ThreadState::Stopped;
}

void ThreadBase::stop()
{
    _state = ThreadState::Stopping;
}

void ThreadBase::join()
{
    while (!isStopped())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    _threadPtr->join();
    _threadPtr.reset(nullptr);
}

bool ThreadBase::isStopping() const noexcept
{
    return _state == ThreadState::Stopping;
}

bool ThreadBase::isStopped() const noexcept
{
    return _state == ThreadState::Stopped;
}

void ThreadBase::sleep(int64_t ms)
{
    PARROT_ASSERT(ms != 0);

    if (std::this_thread::get_id() != getThreadId())
    {
        // Only this thread can call sleep.
        return;
    }

    // Acquire _lock.
    std::unique_lock<std::mutex> lk(_lock);
    _sleeping = true;

    if (ms > 0)
    {
        // Release _lock. Block this thread.
        _condVar.wait_for(lk, std::chrono::milliseconds(ms), [this]()
                          {
                              return !_sleeping;
                          });
    }
    else
    {
        _condVar.wait(lk, [this]()
                      {
                          return !_sleeping;
                      });
    }

    // Here, Reacquire _lock again.
    _sleeping = false;

    // Release the lock;
    lk.unlock();
}

std::thread::id ThreadBase::getThreadId() const
{
    return _threadPtr->get_id();
}

void ThreadBase::wakeUp()
{
    std::unique_lock<std::mutex> lk(_lock);
    _sleeping = false;

    // Release the lock.
    lk.unlock();

    _condVar.notify_one();
}
}
