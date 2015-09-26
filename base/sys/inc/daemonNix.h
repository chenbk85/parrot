#ifndef __BASE_SYS_INC_DAEMONNIX_H__
#define __BASE_SYS_INC_DAEMONNIX_H__

#include "config.h"

namespace parrot {
/**
 * Daemon impl for nix systems.
 */
class DaemonNix {
  public:
    DaemonNix(const Config *);

  public:
    void daemonize();
    void shutdown();

  private:
    void fork();
    void redirectStdIO();
    void writePidFile();

  private:
    const Config *_config;
};
}

#endif
