#include <system_error>
#include "rwLockImpl.h"

namespace parrot {
RwLockImpl::RwLockImpl() : _rwLock() {
    int err = pthread_rwlock_init(&_rwLock, nullptr);
    if (err != 0) {
        pthread_rwlock_destroy(&_rwLock);
        throw std::system_error(err, std::system_category(),
                                "RwLockImpl::RwLockImpl");
    }
}

RwLockImpl::~RwLockImpl() {
    pthread_rwlock_destroy(&_rwLock);
}

void RwLockImpl::lockRead() {
    int err = pthread_rwlock_rdlock(&_rwLock);
    if (err != 0) {
        throw std::system_error(err, std::system_category(),
                                "RwLockImpl::lockRead");
    }
}

void RwLockImpl::lockWrite() {
    int err = pthread_rwlock_wrlock(&_rwLock);
    if (err != 0) {
        throw std::system_error(err, std::system_category(),
                                "RwLockImpl::lockWrite");
    }
}

void RwLockImpl::unlock() {
    int err = pthread_rwlock_unlock(&_rwLock);
    if (err != 0) {
        throw std::system_error(err, std::system_category(),
                                "RwLockImpl::unlock");
    }
}
}
