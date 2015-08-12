#ifndef __BASE_SYS_INC_EPOLLIMPL_H__
#define __BASE_SYS_INC_EPOLLIMPL_H__

#if defined(__linux__)

#include <cstdint>
#include <memory>
#include <sys/epoll.h>

namespace parrot
{
    class IoEvent;
    class EventTrigger;
    enum class eIoAction : uint16_t;

    class EpollImpl : public EventNotifier
    {
      public:
        explicit EpollImpl(uint32_t size) noexcept;
        virtual ~EpollImpl();

      public:
        // Create the epoll.
        void create() override;

        // Wait io events for N milliseconds. If interrupt, it will continue
        // waiting.
        //
        // Params:
        // * ms: milliseconds.
        //
        // Return
        // The number of events.
        uint32_t waitIoEvents(int32_t ms) override;

        // Add io event to epoll.
        //
        // Params:
        // * ev: The io event.
        void addEvent(IoEvent *ev) override;

        // Notify read event.
        //
        // Params:
        // * ev: The io event.
        void monitorRead(IoEvent *ev) override;
 
        // Notify write event.
        //
        // Params:
        // * ev: The io event.
        void monitorWrite(IoEvent *ev) override;

        // Delete io event from epoll.
        //
        // Params:
        // * ev: The io event.
        void delEvent(IoEvent *ev) override;

        // Retrieve the need-to-handle event notified by epoll.
        //
        // Params:
        // * idx: The event index.
        //
        // Return:
        //  The IoEvent pointer.
        IoEvent *getIoEvent(uint32_t idx) const noexcept override;

        // Make epoll_wait return by writing to a fd.
        void stopWaiting() override;

      private:
        int32_t                                  _epollFd;
        uint32_t                                 _epollSize;
        std::unique_ptr<EventTrigger>            _trigger;
        std::unique_ptr<struct epoll_event[]>    _events;
    };
}

#endif // #if defined(__linux__)

#endif
