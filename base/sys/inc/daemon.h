#ifndef __BASE_SYS_INC_DAEMON_H__
#define __BASE_SYS_INC_DAEMON_H__

namespace parrot
{
    class Daemon
    {
      public:
        Daemon();
        virtual ~Daemon();

      public:
        void daemonize();
    };
}

#endif
