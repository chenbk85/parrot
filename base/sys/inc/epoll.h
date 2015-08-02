#ifndef __BASE_SYS_INC_EPOLL_H__
#define __BASE_SYS_INC_EPOLL_H__

#include <cstdint>
#include <sys/epoll.h>
#include <memory>
#include "ioEvent.h"
#include "epollTrigger.h"

namespace parrot
{
    class Epoll
    {
      public:
        explicit Epoll(uint32_t size) noexcept;
        ~Epoll();

      public:
        // Create the epoll.
        void create();

        // Wait io events for N milliseconds. If interrupt, it will continue
        // waiting.
        //
        // Params:
        // * ms: milliseconds.
        //
        // Return
        // The number of events.
        int waitIoEvents(uint32_t ms);

        // Add io event to epoll.
        //
        // Params:
        // * ev: The io event.
        // * events: Epoll flags, EPOLLIN|...
        void addEvent(IoEvent *ev, int events);

        // Update io event in epoll.
        //
        // Params:
        // * ev: The io event.
        // * events: Epoll flags, EPOLLIN|...        
        void modifyEvent(IoEvent *ev, int events);
 
        // Delete io event from epoll.
        //
        // Params:
        // * ev: The io event.
        void delEvent(IoEvent *ev);

        // Retrieve the need-to-handle event notified by epoll.
        //
        // Params:
        // * idx: The event index.
        //
        // Return:
        //  The IoEvent pointer.
        IoEvent *getIoEvent(uint32_t idx) const noexcept;

        // Retrieve the events of IoEvent notified by epoll.
        //
        // Params:
        // * idx: The event index.
        //
        // Return:
        //  The epoll events.
        int getEvents(uint32_t idx) const noexcept;

        // Make epoll_wait return by writing to a fd.
        void stopWaiting();

      private:
        int32_t                                  _epollFd;
        uint32_t                                 _epollSize;
        std::unique_ptr<EpollTrigger>            _trigger;
        std::unique_ptr<struct epoll_events[]>   _events;
    };
}

#endif
