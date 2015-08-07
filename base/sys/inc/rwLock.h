#ifndef __BASE_SYS_INC_RWLOCK_H__
#define __BASE_SYS_INC_RWLOCK_H__

namespace parrot
{
    class RwLockImpl;

    class RwLock
    {
      public:
        RwLock();
        ~RwLock();
        RwLock(const RwLock &) = delete;
        RwLock& operator=(const RwLock &) = delete;

      public:
        void lockRead();
        void lockWrite();
        void unlock();

      private:
        RwLockImpl *_lock;
    };

    class ReadLockGuard
    {
      public:
        ReadLockGuard(RwLock &lock);            
        ~ReadLockGuard();
        ReadLockGuard(const ReadLockGuard &) = delete;
        ReadLockGuard& operator=(const ReadLockGuard &) = delete;

      private:
        RwLock &_lock;
    };

    class WriteLockGuard
    {
      public:
        WriteLockGuard(RwLock &lock);
        ~WriteLockGuard();
        WriteLockGuard(const WriteLockGuard &) = delete;
        WriteLockGuard& operator=(const WriteLockGuard &) = delete;

      private:
        RwLock &_lock;
    };
}

#endif
