#ifndef __BASE_SYS_INC_RWLOCK_H__
#define __BASE_SYS_INC_RWLOCK_H__

#include <pthread.h>

namespace parrot
{ 
    namespace base
    {
        class RwLock
        {
          public:
            RwLock();
            ~RwLock();
            RwLock(const RwLock&) = delete;
            RwLock(RwLock &&) = delete;

          public:
            void lockRead();
            void lockWrite();
            void unlock();

          private:
            pthread_rwlock_t _rwLock;
        };

        class ReadLockGuard
        {
          public:
            ReadLockGuard(RwLock &lock);            
            ~ReadLockGuard();
            ReadLockGuard(const ReadLockGuard &) = delete;
            ReadLockGuard(ReadLockGuard &&) = delete;

          private:
            RwLock &_lock;
        };

        class WriteLockGuard
        {
          public:
            WriteLockGuard(RwLock &lock);
            ~WriteLockGuard();
            WriteLockGuard(const WriteLockGuard &) = delete;
            WriteLockGuard(WriteLockGuard &&) = delete;

          private:
            RwLock &_lock;
        };
    } 
}

#endif
