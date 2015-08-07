#include "ioEvent.h"
#include "epollImpl.h"
#include "epoll.h"

namespace parrot
{
    Epoll::Epoll(uint32_t size) noexcept:
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

    int Epoll::waitIoEvents(int32_t ms)
    {
        return _epollImpl->waitIoEvents(ms);
    }

    void Epoll::addEvent(IoEvent *ev, int events)
    {
        _epollImpl->addEvent(ev, events);
    }
        
    void Epoll::modifyEvent(IoEvent *ev, int events)
    {
        _epollImpl->modifyEvent(ev, events);
    }
    void Epoll::delEvent(IoEvent *ev)
    {
        _epollImpl->delEvent(ev);
    }

    IoEvent* Epoll::getIoEvent(uint32_t idx) const noexcept
    {
        return _epollImpl->getIoEvent(idx);
    }

    int Epoll::getEvents(uint32_t idx) const noexcept
    {
        return _epollImpl->getEvents(idx);
    }

    void Epoll::stopWaiting()
    {
        _epollImpl->stopWaiting();
    }
}
