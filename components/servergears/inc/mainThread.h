#ifndef __COMPONENTS_SERVERGEAR_INC_MAINTHREAD_H__
#define __COMPONENTS_SERVERGEAR_INC_MAINTHREAD_H__

#include <string>
#include <cstdint>

namespace parrot
{
class Config;

class MainThread
{
  public:
    explicit MainThread(const Config& cfg);

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
    virtual void createUserThreads() {}
    virtual void run();
    virutal void beforeTerminate();

  protected:
    void createSysThreads();
    void createThreads();
    void daemonize();

  protected:
    const Config& _config;
};
}

#endif
