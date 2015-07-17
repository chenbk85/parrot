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

        class ReadLockHelper
        {
          public:
            ReadLockHelper(RwLock &lock);            
            ~ReadLockHelper();
            ReadLockHelper(const ReadLockHelper &) = delete;
            ReadLockHelper(ReadLockHelper &&) = delete;

          private:
            RwLock &_lock;
        };

        class WriteLockHelper
        {
          public:
            WriteLockHelper(RwLock &lock);
            ~WriteLockHelper();
            WriteLockHelper(const WriteLockHelper &) = delete;
            WriteLockHelper(WriteLockHelper &&) = delete;

          private:
            RwLock &_lock;
        };
    } 
}

#endif
