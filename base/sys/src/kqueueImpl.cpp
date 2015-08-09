#if defined(__APPLE__)

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include <system_error>
#include <chrono>

namespace parrot
{
    KqueueImpl::KqueueImpl(uint32_t maxEvCount) noexcept:
        _kqueueFd(-1),
        _keventCount((maxEvCount + 1) * 2), // +1 for the trigger. 
        _trigger(new EventTrigger()),
        _events(new struct kevent[_keventCount])
    {
        // Kqueue doesn't support updating filter. So to update a filter,
        // we need to remove it from kqueue and add the event with new filter
        // again. For this system, this behaviour will be unacceptable.
        // So I will add <fd, EVFILT_READ> and <fd, EVFILT_WRITE> at
        // the same time, and set EV_DISABLE/EV_ENABLE to <fd, EVFILT_WRITE>
        // to handle write event. That's why
        // _keventCount = (maxEvCount + 1) * 2;
    }

    KqueueImpl::~KqueueImpl()
    {
        closeKqueue();
    }

    void KqueueImpl::create()
    {
        _kqueueFd = ::kqueue();
        if (_kqueueFd == -1)
        {
            throw std::system_error(errno, std::system_category(),
                                    "KqueueImpl::create");            
        }
    }

    void KqueueImpl::msToTimespec(struct timespec *ts, uint32_t ms)
    {
        if (ts == nullptr)
        {
            return;
        }
        
        ts->tv_sec = ms / 1000;
        ts->tv_nsec = (ms % 1000) * 1000000;
    }

    uint32_t KqueueImpl::waitIoEvents(int32_t ms)
    {
        std::unique_ptr<struct timespec> needWait;
        std::chrono::time_point<std::chrono::system_clock> waitTo;
        int ret = 0;

        if (ms >= 0)
        {
            waitTo = std::chrono::system_clock::now() +
                std::chrono::milliseconds(ms);
            needWait.reset(new struct timespec());
            msToTimespec(needWait.get(), ms);
        }

        while (true)
        {
            ret = ::kqueue(_kqueueFd, nullptr, 0, _events.get(),
                           _keventCount, needWait.get());
            if (ret >= 0)
            {
                break;
            }

            if (errno == EINTR)
            {
                if (ms < 0)
                {
                    continue;
                }

                auto curr = std::chrono::system_clock::now();
                auto leftMs = std::chrono::duration_cast<
                    std::chrono::milliseconds>(waitTo - curr).count();

                needWait.reset(new struct timespec());
                msToTimespec(needWait.get(), leftMs);
            }
        }

        return ret;
    }

    void KqueueImpl::addEvent(IoEvent *ev, int filter)
    {
        int fd = ev->getFd();
        if (fd < 0)
        {
#if defined(DEBUG)            
            PARROT_ASSERT(0);
#endif
            return;
        }
        
        struct kevent ke;
        ::EV_SET(&ke, fd, filter, EV_ADD, 0, 0, ev);

        int ret = ::kevent(_kqueueFd, &ke, 1, nullptr, 0, nullptr);
        if (ret == -1)
        {
            throw std::system_error(errno, std::system_category(),
                                    "KqueueImpl::addEvent");
        }

        ev->setFilter(filter);
    }
    
    void KqueueImpl::updateEventFilter(IoEvent *ev, int filter)
    {
        int oldFilter = ev->getFilter();
        int fd = ev->getFd();

        if (fd < 0)
        {
#if defined(DEBUG)            
            PARROT_ASSERT(0);
#endif
            return;
        }

        if (filter < 0)
        {
#if defined(DEBUG)            
            PARROT_ASSERT(0);
#endif
            return;
        }

        struct kevent ke;
        ::EV_SET(&ke, fd, oldFilter, EV_DELETE, 0, 0, ev);
        
        int ret = ::kevent(_kqueueFd, &ke, 1, nullptr, 0, nullptr);
        if (ret == -1)
        {
            throw std::system_error(errno, std::system_category(),
                                    "KqueueImpl::updateEventFilter:remove");
        }

        if (ret != 1)
        {
#if defined(DEBUG)            
            PARROT_ASSERT(0);
#endif
            return;
        }

        ::EV_SET(&ke, fd, oldFilter, EV_ADD, 0, 0, ev);
        ret = ::kevent(_kqueueFd, &ke, 1, nullptr, 0, nullptr);
        if (ret == -1)
        {
            throw std::system_error(errno, std::system_category(),
                                    "KqueueImpl::updateEventFilter:add");
        }
    }
    
    void KqueueImpl::delEvent(IoEvent *ev)
    {
        int fd = ev->getFd();
        if (fd < 0)
        {
#if defined(DEBUG)            
            PARROT_ASSERT(0);
#endif
            return;
        }

        int filter = ev->getFilter();
        if (filter < 0)
        {
#if defined(DEBUG)            
            PARROT_ASSERT(0);
#endif
            return;
        }

        struct kevent ke;
        ::EV_SET(&ke, fd, filter, EV_DELETE, 0, 0, ev);
        
        int ret = ::kevent(_kqueueFd, &ke, 1, nullptr, 0, nullptr);
        if (ret == -1)
        {
            throw std::system_error(errno, std::system_category(),
                                    "KqueueImpl::delEvent");
        }
        
        if (ret != 1)
        {
#if defined(DEBUG)            
            PARROT_ASSERT(0);
#endif
        }
    }
    
    IoEvent * KqueueImpl::getIoEvent(uint32_t idx) const noexcept
    {
        return (IoEvent*)_events[idx].udata;
    }

    int KqueueImpl::getFilter(uint32_t idx) const noexcept
    {
        return _events[idx].flags;
    }

    void KqueueImpl::closeKqueue()
    {
        if (_kqueueFd != -1)
        {
            ::close(_kqueueFd);
            _kqueueFd = -1;
        }
    }
}

#endif
