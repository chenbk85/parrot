#ifndef __BASE_SYS_INC_DAEMONNIX_H__
#define __BASE_SYS_INC_DAEMONNIX_H__

#include <functional>

#include "daemonBase.h"

namespace parrot
{
struct Config;

/**
 * Daemon impl for nix systems.
 */
class DaemonNix : public DaemonBase
{
  public:
    DaemonNix();

  public:
    static DaemonNix& getInstance();

  public:

    // daemonize
    //
    // Make the program as a daemon.
    void daemonize() override;

    // beforeCreateThreads
    //
    // This function blocks all signals and register signal handler.
    // Create threads after calling this function, the created threads
    // will inherit the main thread's signal mask, a.k.a, blocks all
    // signals.
    void beforeCreateThreads() override;

    // afterCreateThreads
    //
    // This function unblocks all signals. Since the sub threads already
    // inherit the main thread's signal mask, we can unblock all signal.
    // By doing so, all signals will be delivered to main thread.
    void afterCreateThreads() override;

    // isShutdown
    //
    // Check if received shutdown request.
    bool isShutdown() const override;

    // beforeTerminate
    //
    // This function will remove the lock file created in daemonize
    // function.
    void beforeTerminate() override;

    // shutdownDaemon
    //
    // Client should not call this function. This function is a callback
    // which will be called when process received SIGTERM or SIGINT.
    void shutdownDaemon();

  private:
    void blockAllSignals();
    void unblockAllSignals();
    void handleSignal();

  private:
    int _lockFd;
    volatile bool _isShutdown;
};
}

#endif
