#ifndef __BASE_SYS_INC_EPOLL_H__
#define __BASE_SYS_INC_EPOLL_H__

#if defined (__linux__)
#include <cstdint>

#include "eventNotifier.h"

namespace parrot
{
    class EpollImpl;
    class IoEvent;

    class Epoll final : public EventNotifier
    {
      public:
        explicit Epoll(uint32_t size) noexcept;
        virtual ~Epoll();

      public:
        void create() override;
        uint32_t waitIoEvents(int32_t ms) override;
        void addEvent(IoEvent *ev) override;
        void monitorRead(IoEvent *ev) override;
        void monitorWrite(IoEvent *ev) override;
        void delEvent(IoEvent *ev) override;
        IoEvent *getIoEvent(uint32_t idx) const noexcept override;
        void stopWaiting() override;

      private:
        EpollImpl *_epollImpl;
    };
}

#endif
#endif
