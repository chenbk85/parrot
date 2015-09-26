#include <system_error>
#include "rwLockImpl.h"
#include "rwLock.h"

namespace parrot {
RwLock::RwLock() : _lock(new RwLockImpl()) {
}

RwLock::~RwLock() {
    delete _lock;
    _lock = nullptr;
}

void RwLock::lockRead() {
    _lock->lockRead();
}

void RwLock::lockWrite() {
    _lock->lockWrite();
}

void RwLock::unlock() {
    _lock->unlock();
}

//////////////////////////////////////////////////////////////
/// ReadLockGuard
/////////////////////
ReadLockGuard::ReadLockGuard(RwLock &lock) : _lock(lock) {
    _lock.lockRead();
}

ReadLockGuard::~ReadLockGuard() {
    _lock.unlock();
}

//////////////////////////////////////////////////////////////
/// WriteLockGuard
/////////////////////
WriteLockGuard::WriteLockGuard(RwLock &lock) : _lock(lock) {
    _lock.lockWrite();
}

WriteLockGuard::~WriteLockGuard() {
    _lock.unlock();
}
}
