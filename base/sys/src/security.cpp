#include "security.h"
#include <vector>
#include <mutex>
#include <functional>

////////////////////////////////////////////////////////////////////////////
/// Static locking.
//////////////
static std::vector<mutex*> gLockVec;

static void lockingFunctionCallback(int mode, int n, 
                                    const char * /*file*/, int /*line*/)
{
    if (mode & CRYPTO_LOCK)
    {
        gLockVec[n]->lock();
    }
    else
    {
        gLockVec[n]->unlock();
    }
}

static void threadIdFuncCallback(CRYPTO_THREADID *id)
{
    std::hash<std::thread::id> hasher;
    id->val = (unsigned long)hasher(std::this_thread::get_id());
}

////////////////////////////////////////////////////////////////////////////
/// Dynamic locking.
//////////////
struct CRYPTO_dynlock_value
{
    std::mutex _lock;
};

static struct CRYPTO_dynlock_value * dynCreateFunctionCallback(
    const char */*file*/, int /*line*/)
{
    return new CRYPTO_dynlock_value();
}

static void dynLockFunctionCallback(int mode, struct CRYPTO_dynlock_value *l,
                              const char */*file*/, int /*line*/)
{
    if (mode & CRYPTO_LOCK)
    {
        l->_lock.lock();
    }
    else
    {
        l->_lock.unlock();
    }
}

static void dynDestroyFunctionCallback(struct CRYPTO_dynlock_value *l,
                                 const char */*file*/, int /*line*/)
{
    delete l;
}

namespace parrot
{
    Security::Security() = default;
    Security::~Security() = default;

    void Security::init()
    {
        // Create locks for static lock functions.
        int count = CRYPTO_num_locks();
        gLockVec.reserve(count);
        for (int i = 0; i != count; ++i)
        {
            gLockVec.push_back(new std::mutex());
        }

        // Register static lock callbacks;
        CRYPTO_set_locking_callback(lockingFunctionCallback);
        CRYPTO_THREADID_set_callback(threadIdFuncCallback);

        // Register dynamic lock callbacks.
        CRYPTO_set_dynlock_create_callback(dynCreateFunctionCallback);
        CRYPTO_set_dynlock_lock_callback(dynLockFunctionCallback);
        CRYPTO_set_dynlock_destroy_callback(dynDestroyFunctionCallback);


        SSL_library_init();
        SSL_load_error_strings();
    }

    void Security::deinit()
    {
        // First, clear callbacks.
        CRYPTO_set_locking_callback(nullptr);
        CRYPTO_THREADID_set_callback(nullptr);
        CRYPTO_set_dynlock_create_callback(nullptr);
        CRYPTO_set_dynlock_lock_callback(nullptr);
        CRYPTO_set_dynlock_destroy_callback(nullptr);

        // Second, delete locks.
        for (auto it = gLockVec.begin(); it != gLockVec.end(); ++it)
        {
            delete *it;
        }

        gLockVec.clear();
    }
}
