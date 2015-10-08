#ifndef __BASE_SYS_INC_DAEMONBASE_H__
#define __BASE_SYS_INC_DAEMONBASE_H__

#include <functional>

namespace parrot
{
class Config;

class DaemonBase
{
  public:
    DaemonBase();
    virtual ~DaemonBase() = default;

  public:
    void setConfig(const Config* cfg);

    // registerShutdownCb
    //
    // Register a callback function which will be called when your
    // daemon is shutting down.
    void registerShutdownCb(std::function<void()> &&cb);

    // daemonize
    //
    // Make the program as a daemon.
    virtual void daemonize() = 0;

    // beforeCreateThreads
    //
    // A function will be called before creating threads.
    virtual void beforeCreateThreads() = 0;

    // afterCreateThreads
    //
    // A function will be called after creating threads.
    virtual void afterCreateThreads() = 0;

    // isShutdown
    //
    // Check if received shutdown request.
    virtual bool isShutdown() const = 0;

    // beforeTerminate
    //
    // This function will be called before terminating the daemon.
    virtual void beforeTerminate() = 0;

  protected:
    std::function<void()> _shutdownCb;
    const Config* _config;
};
}

#endif
