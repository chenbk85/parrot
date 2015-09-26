#include "ioEvent.h"
#include "eventNotifier.h"
#include "simpleEventNotifierImpl.h"
#include "simpleEventNotifier.h"

namespace parrot {
SimpleEventNotifier::SimpleEventNotifier()
    : EventNotifier(), _impl(new SimpleEventNotifierImpl()) {
}

SimpleEventNotifier::~SimpleEventNotifier() {
    delete _impl;
    _impl = nullptr;
}

uint32_t SimpleEventNotifier::waitIoEvents(int32_t ms) {
    return _impl->waitIoEvents(ms);
}

IoEvent *SimpleEventNotifier::getIoEvent(uint32_t) const {
    return nullptr;
}

void SimpleEventNotifier::stopWaiting() {
    _impl->stopWaiting();
}
}
