#ifndef __BASE_SYS_INC_DAEMONNIX_H__
#define __BASE_SYS_INC_DAEMONNIX_H__

#include <functional>

namespace parrot
{
struct Config;

/**
 * Daemon impl for nix systems.
 */
class DaemonNix
{
  public:
    DaemonNix();

  public:
    static DaemonNix& getInstance();

  public:
    void setConfig(const Config* cfg);

    // registerShutdownCb
    //
    // Register a callback function which will be called when your
    // daemon is shutting down.
    void registerShutdownCb(std::function<void()> cb);

    // daemonize
    //
    // Make the program as a daemon.
    void daemonize();

    // beforeThreadsStart
    //
    // This function blocks all signals and register signal handler.
    // Create threads after calling this function, the created threads
    // will inherit the main thread's signal mask, a.k.a, blocks all
    // signals.
    void beforeThreadsStart();

    // afterThreadsStart
    //
    // This function unblocks all signals. Since the sub threads already
    // inherit the main thread's signal mask, we can unblock all signal.
    // By doing so, all signals will be delivered to main thread.
    void afterThreadsStart();

    // isShutdown
    //
    // Check if received shutdown request.
    bool isShutdown() const;

    // shutdown
    //
    // This function will remove the lock file created in daemonize
    // function.
    void shutdown();

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
    std::function<void()> _shutdownCb;
    const Config* _config;
};
}

#endif
