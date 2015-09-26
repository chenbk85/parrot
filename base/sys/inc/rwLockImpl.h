#ifndef __BASE_SYS_INC_RWLOCKIMPL_H__
#define __BASE_SYS_INC_RWLOCKIMPL_H__

#include <pthread.h>

namespace parrot {
class RwLockImpl {
  public:
    RwLockImpl();
    ~RwLockImpl();
    RwLockImpl(const RwLockImpl &) = delete;
    RwLockImpl(RwLockImpl &&) = delete;

  public:
    void lockRead();
    void lockWrite();
    void unlock();

  private:
    pthread_rwlock_t _rwLock;
};
}

#endif
