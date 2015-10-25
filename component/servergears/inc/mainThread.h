#ifndef __COMPONENT_SERVERGEAR_INC_MAINTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_MAINTHREAD_H__

#include <string>
#include <cstdint>

namespace parrot
{
struct Config;
class FrontThread;

class MainThread
{
  public:
    using FrontThreadPool = std::unique_ptr<ThreadPool<FrontThread>>;

    
  public:
    explicit MainThread(const Config& cfg);
    virtual ~MainThread() = default;

  public:
    // onStop
    //
    // onStop is the callback which will be called when the
    // process received the shutdown signal.
    void onStop();
    
  public:
    virtual void start();

  protected:
    virtual void beforeStart();
    virtual void createUserThreads() = 0;
    virtual void run();
    virtual void beforeTerminate();

  protected:
    void createSysThreads();
    void createThreads();
    void daemonize();

  protected:
    const Config& _config;
    FrontThreadPool _frontThreadPool;
};
}

#endif
