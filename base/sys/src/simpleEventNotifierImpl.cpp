#include "simpleEventNotifierImpl.h"
#include <chrono>

namespace parrot {
SimpleEventNotifierImpl::SimpleEventNotifierImpl()
    : _waiting(false), _signalCount(0), _lock(), _condVar() {
}

SimpleEventNotifierImpl::~SimpleEventNotifierImpl() {
}

uint32_t SimpleEventNotifierImpl::waitIoEvents(int32_t ms) {
    // Acquire _lock.
    std::unique_lock<std::mutex> lk(_lock);
    if (_signalCount > 0) {
        _signalCount = 0;
        lk.unlock();
        return 0;
    }

    _waiting = true;

    if (ms > 0) {
        // Release _lock. Block this thread.
        _condVar.wait_for(lk, std::chrono::milliseconds(ms),
                          [this]() { return !_waiting; });
    } else {
        _condVar.wait(lk, [this]() { return !_waiting; });
    }

    // Here, Reacquire _lock again.
    _waiting = false;

    // Reset signal count.
    _signalCount = 0;

    // Release the lock;
    lk.unlock();

    return 0;
}

void SimpleEventNotifierImpl::stopWaiting() {
    std::unique_lock<std::mutex> lk(_lock);
    _waiting = false;
    ++_signalCount;
    lk.unlock();

    _condVar.notify_one();
}
}
