#include <system_error>

#include "rwLock.h"

namespace parrot
{
    namespace base
    {
        RwLock::RwLock():
            _rwLock()
        {
            int err = pthread_rwlock_init(&_rwLock, nullptr);
            if (err != 0)
            {
                pthread_rwlock_destroy(&_rwLock);
                throw std::system_error(err, std::system_category(), 
                                        "RwLock::RwLock");
            }
        }

        RwLock::~RwLock()
        {
            pthread_rwlock_destroy(&_rwLock);
        }

        void RwLock::lockRead()
        {
            int err = pthread_rwlock_rdlock(&_rwLock);
            if (err != 0)
            {
                throw std::system_error(err, std::system_category(), 
                    "RwLock::lockRead");
            }
        }

        void RwLock::lockWrite()
        {
            int err = pthread_rwlock_wrlock(&_rwLock);
            if (err != 0)
            {
                throw std::system_error(err, std::system_category(), 
                                        "RwLock::lockWrite");
            }
        }

        void RwLock::unlock()
        {
            int err = pthread_rwlock_unlock(&_rwLock);
            if (err != 0)
            {
                throw std::system_error(err, std::system_category(), 
                                        "RwLock::unlock");
            }
        }

        //////////////////////////////////////////////////////////////
        /// ReadLockGuard
        /////////////////////
        ReadLockGuard::ReadLockGuard(RwLock &lock):
            _lock(lock)
        {
            _lock.lockRead();
        }

        ReadLockGuard::~ReadLockGuard()
        {
            _lock.unlock();
        }

        //////////////////////////////////////////////////////////////
        /// WriteLockGuard
        /////////////////////
        WriteLockGuard::WriteLockGuard(RwLock &lock):
            _lock(lock)
        {
            _lock.lockWrite();
        }

        WriteLockGuard::~WriteLockGuard()
        {
            _lock.unlock();
        }
    }
}
