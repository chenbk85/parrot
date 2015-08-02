#include <cerrno>
#include "epoll.h"

namespace parrot
{
    Epoll::Epoll(uint32_t size) noexcept:
        _epollFd(-1),
        _epollSize(size),
        _trigger(),
        _events()
    {
    }

    Epoll::~Epoll()
    {
        if (_epollFd >= 0)
        {
            ::close(_epollFd);
            _epollFd = -1;
        }
    }

    void Epoll::create()
    {
        _epollFd = ::epoll_create(_epollSize);
        if (_epollFd < 0)
        {
            throw std::system_error(errno, std::system_category(), 
                                    "Epoll::create");
        }

        _events = std::unique_ptr<struct epoll_event[]>(
            new struct epoll_evnet[_epollSize]);

        trigger = std::unique_ptr<EpollTrigger>(new EpollTrigger());
        addEvent(trigger.get(), EPOLLIN);
    }

    void Epoll::waitIoEvents(uint32_t ms)
    {
        std::time_t waitTo = std::time(nullptr) * 1000 + ms;
        int ret = 0;
        
        while (true)
        {
            ret = ::epoll_wait(_epollFd, _events.get(),
                                   _epollSize, needWait);
            if (ret >= 0)
            {
                break;
            }
            
            if (errno == EINTR)
            {
                std::time_t curr = std::time(nullptr) * 1000;
                if (curr >= waitTo)
                {
                    break;
                }
            }
            else
            {
                throw std::system_error(errno, std::system_category(), 
                                        "Epoll::waitIoEvents");
            }
        }

        return ret;
    }

    void Epoll::addEvent(IoEvents *ev, int events)
    {
        int fd = ev->getFd();
        if (fd == -1)
        {
#if defined(DEBUG)            
            PARROT_ASSERT(0);
#endif
            return;
        }

        int oldEvents = ev->getEpollEvents();
        if (oldEvents != -1)
        {
#if defined(DEBUG)            
            PARROT_ASSERT(0);
#endif
            return;            
        }

        struct epoll_event event;
        event.data.u64 = 0;
        event.data.ptr = ev;
        event.events = events;

        int ret = ::epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &event);

        if (ret < 0)
        {
            throw std::system_error(errno, std::system_category(),
                                    "Epoll::addEvent");
        }

        ev->setEpollEvents(events);
    }

    void Epoll::modifyEvent(IoEvents *ev, int events)
    {
        int fd = ev->getFd();
        if (fd == -1)
        {
#if defined(DEBUG)            
            PARROT_ASSERT(0);
#endif
            return;
        }

        int oldEvents = ev->getEpollEvents();
        if (oldEvents == -1)
        {
#if defined(DEBUG)            
            PARROT_ASSERT(0);
#endif
            return;            
        }

        struct epoll_event event;
        event.data.u64 = 0;
        event.data.ptr = ev;
        event.events = events;

        int ret = ::epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &event);

        if (ret < 0)
        {
            throw std::system_error(errno, std::system_category(),
                                    "Epoll::modifyEvent");
        }
        ev->setEpollEvents(events);
    }

    void Epoll::delEvent(IoEvents *ev)
    {
        int fd = ev->getFd();
        if (fd == -1)
        {
#if defined(DEBUG)            
            PARROT_ASSERT(0);
#endif
            return;
        }

        int oldEvents = ev->getEpollEvents();
        if (oldEvents == -1)
        {
#if defined(DEBUG)            
            PARROT_ASSERT(0);
#endif
            return;            
        }

        int ret = ::epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, nullptr);
        if (ret < 0)
        {
            throw std::system_error(errno, std::system_category(),
                                    "Epoll::delEvent");
        }
        ev->setEpollEvents(-1);
    }

    IoEvent* Epoll::getIoEvent(uint32_t idx) const noexcept
    {
        return (IoEvent *)_events[idx].ptr;
    }

    int Epoll::getEvents(uint32_t) const noexcept
    {
        return _events[idx].events;
    }

    void Epoll::stopWaiting()
    {
        trigger->trigger();
    }
}
