#if defined(__APPLE__)
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>

#include <system_error>
#include <chrono>

#include "ioEvent.h"
#include "kqueueImpl.h"
#include "eventTrigger.h"
#include "macroFuncs.h"

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
        close();
    }

    void KqueueImpl::create()
    {
        _kqueueFd = ::kqueue();
        if (_kqueueFd == -1)
        {
            throw std::system_error(errno, std::system_category(),
                                    "KqueueImpl::create");            
        }

        _trigger->create();
        _trigger->setAction(eIoAction::Read);
        addEvent(_trigger.get());
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
            ret = ::kevent(_kqueueFd, nullptr, 0, _events.get(),
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

    void KqueueImpl::addEvent(IoEvent *ev)
    {
        int fd = ev->getFd();
        if (fd < 0)
        {
#if defined(DEBUG)            
            PARROT_ASSERT(0);
#endif
            return;
        }
        
        struct kevent kev[2];
        eIoAction act = ev->getAction();

        if (act == eIoAction::Read)
        {
            ev->setIoRead();
            EV_SET(&kev[0], fd, EVFILT_WRITE, EV_ADD|EV_DISABLE, 0, 0, ev);
            EV_SET(&kev[1], fd, EVFILT_READ, EV_ADD|EV_ENABLE, 0, 0, ev);
        }
        else if (act == eIoAction::Write)
        {
            ev->setIoWrite();
            EV_SET(&kev[0], fd, EVFILT_WRITE, EV_ADD|EV_ENABLE, 0, 0, ev);
            EV_SET(&kev[1], fd, EVFILT_READ, EV_ADD|EV_DISABLE, 0, 0, ev);
        }
        else
        {
#if defined(DEBUG)            
            PARROT_ASSERT(0);
#endif
            return;
        }

        int ret = ::kevent(_kqueueFd, kev, 2, nullptr, 0, nullptr);
        if (ret == -1)
        {
            throw std::system_error(errno, std::system_category(),
                                    "KqueueImpl::addEvent");
        }
    }

    void KqueueImpl::monitorRead(IoEvent *ev)
    {
        int fd = ev->getFd();

        if (fd < 0)
        {
#if defined(DEBUG)            
            PARROT_ASSERT(0);
#endif
            return;
        }

        if (ev->getAction() == eIoAction::Read)
        {
            return;
        }

        // Set the event to READ.
        ev->setIoRead();

        struct kevent kev[2];
        EV_SET(&kev[0], fd, EVFILT_WRITE, EV_ADD|EV_DISABLE, 0, 0, ev);
        EV_SET(&kev[1], fd, EVFILT_READ, EV_ADD|EV_ENABLE, 0, 0, ev);

        int ret = ::kevent(_kqueueFd, kev, 2, nullptr, 0, nullptr);
        if (ret == -1)
        {
            throw std::system_error(errno, std::system_category(),
                                    "KqueueImpl::monitorRead");
        }
    }

    void KqueueImpl::monitorWrite(IoEvent *ev)
    {
        int fd = ev->getFd();

        if (fd < 0)
        {
#if defined(DEBUG)            
            PARROT_ASSERT(0);
#endif
            return;
        }

        if (ev->getAction() == eIoAction::Write)
        {
            return;
        }

        // Set the event to READ.
        ev->setIoWrite();

        struct kevent kev[2];
        EV_SET(&kev[0], fd, EVFILT_WRITE, EV_ADD|EV_ENABLE, 0, 0, ev);
        EV_SET(&kev[1], fd, EVFILT_READ, EV_ADD|EV_DISABLE, 0, 0, ev);

        int ret = ::kevent(_kqueueFd, kev, 2, nullptr, 0, nullptr);
        if (ret == -1)
        {
            throw std::system_error(errno, std::system_category(),
                                    "KqueueImpl::monitorWrite");
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

        struct kevent kev[2];
        EV_SET(&kev[0], fd, EVFILT_WRITE, EV_DELETE, 0, 0, ev);
        EV_SET(&kev[1], fd, EVFILT_READ, EV_DELETE, 0, 0, ev);

        int ret = ::kevent(_kqueueFd, kev, 2, nullptr, 0, nullptr);
        if (ret == -1)
        {
            throw std::system_error(errno, std::system_category(),
                                    "KqueueImpl::delEvent");
        }

        ev->setAction(eIoAction::Remove);
        ev->setFilter(-1);
        ev->setFlags(-1);
    }
    
    IoEvent * KqueueImpl::getIoEvent(uint32_t idx) const noexcept
    {
        IoEvent *ev = (IoEvent*)_events[idx].udata;
        ev->setFilter(_events[idx].filter);
        ev->setFlags(_events[idx].flags);
        return ev;
    }

    void KqueueImpl::stopWaiting()
    {
        _trigger->trigger();
    }

    void KqueueImpl::close()
    {
        if (_kqueueFd != -1)
        {
            ::close(_kqueueFd);
            _kqueueFd = -1;
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
}

#endif // __APPLE__
