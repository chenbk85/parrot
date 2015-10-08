#ifndef __BASE_SYS_INC_DAEMON_H__
#define __BASE_SYS_INC_DAEMON_H__

namespace parrot
{
class Config;

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
    static Daemon* _daemon;
};
}

#endif
