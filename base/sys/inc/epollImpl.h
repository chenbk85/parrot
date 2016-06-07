#ifndef __BASE_SYS_INC_EPOLLIMPL_H__
#define __BASE_SYS_INC_EPOLLIMPL_H__

#if defined(__linux__)

#include <cstdint>
#include <memory>
#include <sys/epoll.h>

#include "sysDefinitions.h"

namespace parrot
{
class IoEvent;
class EventTrigger;

class EpollImpl final
{
  public:
    explicit EpollImpl(uint32_t size) noexcept;
    ~EpollImpl();
    EpollImpl(const EpollImpl& kq) = delete;
    EpollImpl& operator=(const EpollImpl& kq) = delete;

  public:
    /**
     * Create the epoll.
     */
    void create();

    /**
     * Wait io events for N milliseconds. If interrupt, it will continue
     * waiting.
     *
     * @param    ms   Milliseconds.
     * @return        The number of events.
     */
    uint32_t waitIoEvents(int32_t ms);

    /**
     * Add io event to epoll.
     *
     * @param    ev   The io event.
     */
    void addEvent(IoEvent* ev);

    /**
     * Update event's action.
     *
     * @param    ev   The io event.
     */    
    void updateEventAction(IoEvent* ev);

    /**
     * Delete io event from epoll.
     *
     * @param    ev   The io event.
     */        
    void delEvent(IoEvent* ev);

    /**
     * Retrieve the need-to-handle event notified by epoll.
     *
     * @param    idx  The event index.
     * @return        The IoEvent pointer.
     */
    IoEvent* getIoEvent(uint32_t idx) const noexcept;

    /**
     * Make epoll_wait return by writing to a fd.
     */
    void stopWaiting();

    /**
     * Make epoll_wait return by writing to a fd.
     */    
    void close();

  private:
    /**
     * Help function to get filter combination from act.
     *
     * @param    act     The io action.
     * @return           Filter combination.
     */
    int getFilter(eIoAction act);

  private:
    int32_t                               _epollFd;
    uint32_t                              _epollSize;
    std::unique_ptr<EventTrigger>         _trigger;
    std::unique_ptr<struct epoll_event[]> _events;
};
}

#endif // #if defined(__linux__)

#endif
