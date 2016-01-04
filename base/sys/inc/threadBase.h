#ifndef __BASE_SYS_INC_THREADBASE_H__
#define __BASE_SYS_INC_THREADBASE_H__

#include <thread>
#include <string>
#include <cstdint>
#include <memory>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace parrot
{
// Base thread class. You must derive this class and implement the
// run() function. Simple usage:
//     class RpcThread : public ThreadBase {} ...
class ThreadBase
{
    enum class ThreadState : uint8_t
    {
        Init,
        Started,
        Sleeping,
        Stopping,
        Stopped
    };

  public:
    ThreadBase();
    virtual ~ThreadBase();
    ThreadBase(const ThreadBase&) = delete;
    ThreadBase& operator=(const ThreadBase&) = delete;

  public:
    // Start this thread. Asynchronize.
    virtual void start();

    // Stop this thread. Asynchronize.
    virtual void stop();

    // Wake up this thread. Synchronize.
    void wakeUp();

    inline bool isStarted() const noexcept
    {
        return _state == ThreadState::Started;
    }

    inline bool isStopping() const noexcept
    {
        return _state == ThreadState::Stopping;
    }

    inline bool isStopped() const noexcept
    {
        return _state == ThreadState::Stopped;
    }

    inline bool isSleeping() const noexcept
    {
        return _state == ThreadState::Sleeping;
    }

    std::thread::id getThreadId() const;

    const std::string& getThreadIdStr();

    // Join the thread. Synchronize.
    void join();

  protected:
    // Sleep this thread for ms milliseconds. Synchronize.
    //
    // Params:
    // * ms  milliseconds. If ms < 0, blocks the current thread
    //       until the condition variable is woken up.
    void sleep(int64_t ms);

    // This function will be called before starting this thread.
    virtual void beforeStart()
    {
    }

    // This function will be called before calling the run() function.
    virtual void beforeRun()
    {
    }

    // The thread entry function.
    virtual void run() = 0;

    // This function will be called before stopping this thread.
    virtual void beforeStop()
    {
    }

  private:
    void entryFunc();

  private:
    std::unique_ptr<std::thread> _threadPtr;
    std::atomic<ThreadState> _state;
    std::mutex _lock;
    std::condition_variable _condVar;
    std::string _tid;
};
}
#endif
