#include <chrono>
#include <iostream>

#include "threadBase.h"
#include "macroFuncs.h"

using namespace std;

namespace parrot
{

ThreadBase::ThreadBase()
    : _threadPtr(), _state(ThreadState::Init), _lock(), _condVar(), _tid()
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
    _threadPtr = std::unique_ptr<std::thread>(
        new std::thread(&ThreadBase::entryFunc, this));
}

void ThreadBase::entryFunc()
{
    _state = ThreadState::Started;
    beforeRun();
    run();
    _state = ThreadState::Stopped;
}

void ThreadBase::stop()
{
    if (isSleeping())
    {
        wakeUp();
    }

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

void ThreadBase::sleep(int64_t ms)
{
    PARROT_ASSERT(ms != 0);
    PARROT_ASSERT(_state == ThreadState::Started);

    if (std::this_thread::get_id() != getThreadId())
    {
        // Only this thread can call sleep.
        return;
    }

    // Acquire _lock.
    std::unique_lock<std::mutex> lk(_lock);
    _state = ThreadState::Sleeping;

    if (ms > 0)
    {
        // Release _lock. Block this thread.
        _condVar.wait_for(lk, std::chrono::milliseconds(ms), [this]()
                          {
                              return _state != ThreadState::Sleeping;
                          });
    }
    else
    {
        _condVar.wait(lk, [this]()
                      {
                          return _state != ThreadState::Sleeping;
                      });
    }

    _state = ThreadState::Started;
    // Release the lock;
    lk.unlock();
}

std::thread::id ThreadBase::getThreadId() const
{
    return _threadPtr->get_id();
}

const std::string& ThreadBase::getThreadIdStr()
{
    if (_tid.empty())
    {
        std::hash<std::thread::id> hasher;
        _tid = std::to_string(hasher(getThreadId()));
    }
    return _tid;
}

void ThreadBase::wakeUp()
{
    std::unique_lock<std::mutex> lk(_lock);
    _state = ThreadState::Init;

    // Release the lock.
    lk.unlock();

    _condVar.notify_one();

    while (!isStarted())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
}
