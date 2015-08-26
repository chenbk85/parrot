#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>

#include <vector>
#include <mutex>
#include <set>
#include <functional>
#include <cstring>

#include "security.h"
#include "macroFuncs.h"
#include "stringHelper.h"

////////////////////////////////////////////////////////////////////////////
/// Static locking.
//////////////
static std::vector<std::mutex*> gLockVec;

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

////////////////////////////////////////////////////////////////////////////
/// Check host name.
//////////////

// Tries to find a match for hostname in the certificate's Common Name field.
//
// Tries to find a match for hostname in the certificate's 
// Subject Alternative Name extension.
static bool matchesCommonName(const std::string &host, const X509 *cert)
{
    int commonNameLoc = -1;
    X509_NAME_ENTRY *commonNameEntry = nullptr;
    ASN1_STRING *commonNameAsn1 = nullptr;
    char *commonNameStr = nullptr;

    // Find the position of the CN field in the Subject field of the certificate
    commonNameLoc = X509_NAME_get_index_by_NID(
        X509_get_subject_name((X509 *) cert), NID_commonName, -1);
    if (commonNameLoc < 0) 
    {
        return false;
    }

    // Extract the CN field
    commonNameEntry = X509_NAME_get_entry(
        X509_get_subject_name((X509 *) cert), commonNameLoc);
    if (commonNameEntry == nullptr) 
    {
        return false;
    }

    // Convert the CN field to a C string
    commonNameAsn1 = X509_NAME_ENTRY_get_data(commonNameEntry);
    if (commonNameAsn1 == nullptr) 
    {
        return false;
    }
    commonNameStr = (char *) ASN1_STRING_data(commonNameAsn1);

    // Make sure there isn't an embedded NUL character in the CN
    if (ASN1_STRING_length(commonNameAsn1) != std::strlen(commonNameStr)) 
    {
        return false;
    }

    // Compare expected hostname with the CN
    if (parrot::iStringCmp(host.c_str(), commonNameStr)) 
    {
        return true;
    }
    else 
    {
        return false;
    }
}

// matchesSubjectAlternativeName
//
// Tries to find a match for hostname in the certificate's 
// Subject Alternative Name extension.
static bool matchesSubjectAlternativeName(const std::string &host, 
                                          const X509 *cert)
{
    int sanNamesNb = -1;
    STACK_OF(GENERAL_NAME) *sanNames = nullptr;

    // Try to extract the names within the SAN extension from the certificate
    sanNames = (stack_st_GENERAL_NAME*)X509_get_ext_d2i(
        (X509 *) cert, NID_subject_alt_name, nullptr, nullptr);
    if (sanNames == nullptr) 
    {
        return false;
    }
    sanNamesNb = sk_GENERAL_NAME_num(sanNames);

    bool res = false;

    // Check each name within the extension
    for (int i = 0; i < sanNamesNb; ++i) 
    {
        const GENERAL_NAME *currentName = sk_GENERAL_NAME_value(sanNames, i);

        if (currentName->type == GEN_DNS) 
        {
            // Current name is a DNS name, let's check it
            char *dnsName = (char *) ASN1_STRING_data(currentName->d.dNSName);

            // Make sure there isn't an embedded NUL character in the DNS name
            if (ASN1_STRING_length(currentName->d.dNSName) != strlen(dnsName))
            {
                res = false;
                break;
            }
            else 
            {
                // Compare expected hostname with the DNS name
                if (parrot::iStringCmp(host.c_str(), dnsName)) 
                {
                    res = true;
                    break;
                }
            }
        }
    }

    sk_GENERAL_NAME_pop_free(sanNames, GENERAL_NAME_free);
    return res;
}

//////////////////////////////////////////////////////////////////////////////
namespace parrot
{
    Security::Security() = default;
    Security::~Security() = default;

    void Security::init()
    {
        OpenSSL_add_all_algorithms();
        OpenSSL_add_all_ciphers();
        OpenSSL_add_all_digests();

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
    }

    void Security::freeThreadErrQueue(const std::thread::id &id)
    {
        CRYPTO_THREADID tid;
        std::hash<std::thread::id> hasher;
        tid.val = (unsigned long)hasher(id);
        ERR_remove_thread_state(&tid);
    }

    SSL_CTX* Security::genSslCtx(const std::string &keyPath, 
                                 const std::string &certPath, 
                                 bool verifyPeer,
                                 int depth)
    {
        const SSL_METHOD *m = TLSv1_method();
        SSL_CTX *sslCtx = SSL_CTX_new(m);

        if (!sslCtx)
        {
            // If here, did you forget to call Security::init()?
            PARROT_ASSERT(0);
        }

        if (SSL_CTX_load_verify_locations(sslCtx, certPath.c_str(), 
                                          nullptr) != 1)
        {
            PARROT_ASSERT(0);
        }

        if (SSL_CTX_set_default_verify_paths(sslCtx) != 1)
        {
            PARROT_ASSERT(0);
        }

        if (SSL_CTX_use_certificate_chain_file(sslCtx, certPath.c_str()) != 1)
        {
            PARROT_ASSERT(0);
        }

        if (SSL_CTX_use_PrivateKey_file(sslCtx, keyPath.c_str(), 
                                        SSL_FILETYPE_PEM) != 1)
        {
            PARROT_ASSERT(0);
        }

        if (verifyPeer) 
        {
            SSL_CTX_set_verify(
                sslCtx, 
                SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT, 
                nullptr);
            SSL_CTX_set_verify_depth(sslCtx, depth);
        }
        else
        {
            SSL_CTX_set_verify(sslCtx, SSL_VERIFY_NONE, nullptr);
        }

        return sslCtx;
    }

    bool checkCertHostname(SSL *ssl, const std::string &host)
    {
        X509 *cert = nullptr;
        if (SSL_get_verify_result(ssl) != X509_V_OK)
        {
            return false;
        }

        if (!(cert = SSL_get_peer_certificate(ssl)) || host.empty())
        {
            return false;
        }

        if (!matchesCommonName(host, cert) || 
            !matchesSubjectAlternativeName(host, cert)) 
        {
            return false;
        }

        X509_free(cert);
        return true;
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
