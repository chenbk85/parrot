#if defined(_WIN32)

#include "winIocpImpl.h"
#include "winIocp.h"

namespace parrot
{
    WinIocp::WinIocp(HANDLE iocp, uint32_t dequeueCount):
        _impl(new WinIocpImpl(iocp, dequeueCount))
    {
    }

    WinIocp::~WinIocp()
    {
        delete _impl;
        _impl = nullptr;
    }

    void WinIocp::create()
    {
    }

    uint32_t WinIocp::waitIoEvents(int32_t ms)
    {
        return _impl->waitIoEvents(ms);
    }

    void WinIocp::addEvent(IoEvent *ev)
    {
        impl->addEvent(ev);
    }
        
    void WinIocp::monitorRead(IoEvent *)
    {
    }

    void WinIocp::monitorWrite(IoEvent *)
    {
    }

    void WinIocp::delEvent(IoEvent *)
    {
    }

    WinIoEvent* WinIocp::getIoEvent(uint32_t idx) const
    {
        return _impl->getIoEvent(idx);
    }

    void stopWaiting()
    {
        _impl->stopWaiting();
    }
}

#endif
