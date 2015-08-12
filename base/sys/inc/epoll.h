#ifndef __BASE_SYS_INC_EPOLL_H__
#define __BASE_SYS_INC_EPOLL_H__

#if defined (__linux__)
#include <cstdint>

namespace parrot
{
    class EpollImpl;
    class IoEvent;

    class Epoll
    {
      public:
        explicit Epoll(uint32_t size) noexcept;
        ~Epoll();

      public:
        void create();
        uint32_t waitIoEvents(int32_t ms);
        void addEvent(IoEvent *ev);
        void monitorRead(IoEvent *ev);
        void monitorWrite(IoEvent *ev);
        void delEvent(IoEvent *ev);
        IoEvent *getIoEvent(uint32_t idx) const noexcept;
        void stopWaiting();

      private:
        EpollImpl *_epollImpl;
    };
}

#endif
#endif
