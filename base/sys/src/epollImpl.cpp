#if defined(__linux__)
#include <cerrno>
#include <chrono>
#include <system_error>

#include <unistd.h>

#include "epollImpl.h"
#include "macroFuncs.h"
#include "ioEvent.h"
#include "eventTrigger.h"

namespace parrot
{
EpollImpl::EpollImpl(uint32_t size) noexcept
    : _epollFd(-1),
      _epollSize(size + 1), // We need add the trigger to epoll.
      _trigger(),
      _events()
{
}

EpollImpl::~EpollImpl()
{
    close();
}

void EpollImpl::create()
{
    _epollFd = ::epoll_create(_epollSize);
    if (_epollFd < 0)
    {
        throw std::system_error(errno, std::system_category(),
                                "EpollImpl::create");
    }

    _events.reset(new struct epoll_event[_epollSize]);
    _trigger = std::unique_ptr<EventTrigger>(new EventTrigger());
    _trigger->create();
    addEvent(_trigger.get());
}

uint32_t EpollImpl::waitIoEvents(int32_t ms)
{
    int32_t needWait = ms;
    std::chrono::time_point<std::chrono::system_clock> waitTo;
    int ret = 0;

    if (ms >= 0)
    {
        waitTo =
            std::chrono::system_clock::now() + std::chrono::milliseconds(ms);
    }

    while (true)
    {
        ret = ::epoll_wait(_epollFd, _events.get(), _epollSize, needWait);
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
            needWait = std::chrono::duration_cast<std::chrono::milliseconds>(
                           waitTo - curr)
                           .count();

            if (needWait <= 0)
            {
                ret = 0;
                break;
            }
        }
        else
        {
            throw std::system_error(errno, std::system_category(),
                                    "EpollImpl::waitIoEvents");
        }
    }

    return ret;
}

int EpollImpl::getFilter(eIoAction act)
{
    int filter = 0;
    switch (act)
    {
        case eIoAction::Read:
        {
            filter = EPOLLIN | EPOLLRDHUP | EPOLLET;
        }
        break;

        case eIoAction::Write:
        {
            filter = EPOLLOUT | EPOLLRDHUP | EPOLLET;
        }
        break;

        case eIoAction::ReadWrite:
        {
            filter = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
        }
        break;

        default:
        {
            PARROT_ASSERT(false);
        }
        break;
    }

    return filter;
}

void EpollImpl::addEvent(IoEvent* ev)
{
    int fd = ev->getFd();
    if (fd == -1)
    {
#if defined(DEBUG)
        PARROT_ASSERT(0);
#endif
        return;
    }

    struct epoll_event event;
    event.data.u64 = 0;
    event.data.ptr = ev;

    eIoAction act = ev->getNextAction();
    ev->setCurrAction(act);
    event.events = getFilter(act);

    int ret = ::epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &event);
    if (ret < 0)
    {
        throw std::system_error(errno, std::system_category(),
                                "EpollImpl::addEvent");
    }
}

void EpollImpl::updateEventAction(IoEvent *ev)
{
    int fd = ev->getFd();
    if (fd == -1)
    {
#if defined(DEBUG)
        PARROT_ASSERT(0);
#endif
        return;
    }

    if (ev->sameAction())
    {
        return;
    }

    eIoAction act = ev->getNextAction();
    ev->setCurrAction(act);

    struct epoll_event event;
    event.data.u64 = 0;
    event.data.ptr = ev;
    event.events = getFilter(act);

    int ret = ::epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &event);

    if (ret < 0)
    {
        throw std::system_error(errno, std::system_category(),
                                "EpollImpl::updateEventAction");
    }
}

void EpollImpl::delEvent(IoEvent* ev)
{
    int fd = ev->getFd();
    if (fd == -1)
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
                                "EpollImpl::delEvent");
    }
}

IoEvent* EpollImpl::getIoEvent(uint32_t idx) const noexcept
{
    IoEvent* ev = (IoEvent*)_events[idx].data.ptr;
    uint32_t filter = _events[idx].events;

    if ((filter & EPOLLERR) || (filter & EPOLLHUP))
    {
        ev->setError(true);
        return ev;
    }

    if ((filter & EPOLLRDHUP) || (filter & EPOLLHUP))
    {
        ev->setEof(true);
        return ev;
    }

    if ((filter & EPOLLIN) && (filter & EPOLLOUT))
    {
        ev->setNotifiedAction(eIoAction::ReadWrite);
    }
    else if (filter & EPOLLIN || filter & EPOLLPRI)
    {
        ev->setNotifiedAction(eIoAction::Read);
    }
    else if (filter & EPOLLOUT)
    {
        ev->setNotifiedAction(eIoAction::Write);
    }
    else
    {
        PARROT_ASSERT(false);
    }
    
    return ev;
}

void EpollImpl::stopWaiting()
{
    _trigger->trigger();
}

void EpollImpl::close()
{
    if (_epollFd >= 0)
    {
        ::close(_epollFd);
        _epollFd = -1;
    }
}
}

#endif // __linux__
