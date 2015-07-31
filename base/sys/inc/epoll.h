#ifndef __BASE_SYS_INC_EPOLL_H__
#define __BASE_SYS_INC_EPOLL_H__

#include "ioEvent.h"

namespace parrot
{
    class Epoll
    {
      public:
        Epoll();
        ~Epoll();

      public:
        void create();
        void waitFor();

      private:
        int _epollFd;
    };
}

#endif
