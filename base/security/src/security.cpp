#include "security.h"

#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <vector>
#include <mutex>
#include <set>
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
            gLockVec.emplace_back(new std::mutex());
        }

        // Register static lock callbacks;
        CRYPTO_set_locking_callback(lockingFunctionCallback);
        CRYPTO_THREADID_set_callback(threadIdFuncCallback);

        // Register dynamic lock callbacks.
        CRYPTO_set_dynlock_create_callback(dynCreateFunctionCallback);
        CRYPTO_set_dynlock_lock_callback(dynLockFunctionCallback);
        CRYPTO_set_dynlock_destroy_callback(dynDestroyFunctionCallback);

        // Init library.
        SSL_library_init();
        SSL_load_error_strings();

        OpenSSL_add_all_algorithms();
        OpenSSL_add_all_ciphers();
        OpenSSL_add_all_digests();
    }

    void Security::freeThreadErrQueue(const std::thread::id &id)
    {
        CRYPTO_THREADID tid;
        std::hash<std::thread::id> hasher;
        tid.val = (unsigned long)hasher(id);
        ERR_remove_thread_state(&tid);
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

        EVP_cleanup();
        CRYPTO_cleanup_all_ex_data();
        ERR_free_strings();
    }
}
