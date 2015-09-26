#if defined(_WIN32)

#include "eventNotifier.h"
#include "winIoEvent.h"
#include "winIocpImpl.h"
#include "winIocp.h"

namespace parrot {
WinIocp::WinIocp(HANDLE iocp, uint32_t dequeueCount)
    : EventNotifier(), _impl(new WinIocpImpl(iocp, dequeueCount)) {
}

WinIocp::~WinIocp() {
    delete _impl;
    _impl = nullptr;
}

HANDLE WinIocp::createIocp(uint32_t threadNum) {
    return WinIocp::createIocp(threadNum);
}

uint32_t WinIocp::waitIoEvents(int32_t ms) {
    return _impl->waitIoEvents(ms);
}

void WinIocp::addEvent(WinIoEvent *ev) {
    impl->addEvent(ev);
}

WinIoEvent *WinIocp::getIoEvent(uint32_t idx) const {
    return _impl->getIoEvent(idx);
}

void stopWaiting() {
    _impl->stopWaiting();
}
}

#endif
