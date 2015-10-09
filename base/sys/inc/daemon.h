#ifndef __BASE_SYS_INC_DAEMON_H__
#define __BASE_SYS_INC_DAEMON_H__

#include <functional>

namespace parrot
{
struct Config;
class DaemonBase;

class Daemon
{
  public:
    static void setConfig(const Config* cfg);
    static void daemonize();
    static void beforeCreateThreads();
    static void afterCreateThreads();
    static bool isShutdown();
    static void beforeTerminate();
    static void registerShutdownCb(std::function<void()> &&cb);

  private:
    static DaemonBase* _daemon;
};
}

#endif
