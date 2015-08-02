#ifndef __BASE_SYS_INC_EPOLL_H__
#define __BASE_SYS_INC_EPOLL_H__

#include <cstdint>
#include <sys/epoll.h>
#include <memory>
#include "ioEvent.h"


namespace parrot
{
    class Epoll
    {
      public:
        explicit Epoll(uint32_t size) noexcept;
        ~Epoll();

      public:
        void create();
        void waitIoEvents();
        void addEvent(IoEvent *ev, int events);
        void modifyEvent(IoEvent *ev, int events);
        void delEvent(IoEvent *ev);

      private:
        int32_t                                  _epollFd;
        uint32_t                                 _epollSize;
        std::unique_ptr<struct epoll_events[]>   _events;
    };
}

#endif
