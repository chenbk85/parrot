#if defined(__linux__)

#include "ioEvent.h"
#include "epollImpl.h"
#include "epoll.h"

namespace parrot
{
    Epoll::Epoll(uint32_t size) noexcept:
        EventNotifier(),
        _epollImpl(new EpollImpl(size))
    {
    }

    Epoll::~Epoll()
    {
        delete _epollImpl;
        _epollImpl = nullptr;
    }

    void Epoll::create()
    {
        _epollImpl->create();
    }

    uint32_t Epoll::waitIoEvents(int32_t ms)
    {
        return _epollImpl->waitIoEvents(ms);
    }

    void Epoll::addEvent(IoEvent *ev)
    {
        _epollImpl->addEvent(ev);
    }

    void Epoll::monitorRead(IoEvent *ev)
    {
        _epollImpl->monitorRead(ev);
    }

    void Epoll::monitorWrite(IoEvent *ev)
    {
        _epollImpl->monitorWrite(ev);
    }

    void Epoll::delEvent(IoEvent *ev)
    {
        _epollImpl->delEvent(ev);
    }

    IoEvent* Epoll::getIoEvent(uint32_t idx) const noexcept
    {
        return _epollImpl->getIoEvent(idx);
    }

    void Epoll::stopWaiting()
    {
        _epollImpl->stopWaiting();
    }
}

#endif
