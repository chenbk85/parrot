#if defined(__APPLE__)

#include "ioEvent.h"
#include "kqueueImpl.h"
#include "kqueue.h"

namespace parrot {
Kqueue::Kqueue(uint32_t size) noexcept : EventNotifier(),
                                         _impl(new KqueueImpl(size)) {
}

Kqueue::~Kqueue() {
    delete _impl;
    _impl = nullptr;
}

void Kqueue::create() {
    _impl->create();
}

uint32_t Kqueue::waitIoEvents(int32_t ms) {
    return _impl->waitIoEvents(ms);
}

void Kqueue::addEvent(IoEvent *ev) {
    _impl->addEvent(ev);
}

void Kqueue::monitorRead(IoEvent *ev) {
    _impl->monitorRead(ev);
}

void Kqueue::monitorWrite(IoEvent *ev) {
    _impl->monitorWrite(ev);
}

void Kqueue::delEvent(IoEvent *ev) {
    _impl->delEvent(ev);
}

IoEvent *Kqueue::getIoEvent(uint32_t idx) const noexcept {
    return _impl->getIoEvent(idx);
}

void Kqueue::stopWaiting() {
    _impl->stopWaiting();
}
}

#endif // __APPLE__
