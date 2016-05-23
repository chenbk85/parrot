/// Important note:
/// The following functions are from the curl project:
///
/// 
///

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

#include <cstring>
#include <functional>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "macroFuncs.h"
#include "sslHelper.h"
#include "stringHelper.h"

////////////////////////////////////////////////////////////////////////////
/// Static locking.
//////////////
static std::vector<std::mutex*> gLockVec;

static void
lockingFunctionCallback(int mode, int n, const char* /*file*/, int /*line*/)
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

static void threadIdFuncCallback(CRYPTO_THREADID* id)
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

static struct CRYPTO_dynlock_value*
dynCreateFunctionCallback(const char* /*file*/, int /*line*/)
{
    return new CRYPTO_dynlock_value();
}

static void dynLockFunctionCallback(int                          mode,
                                    struct CRYPTO_dynlock_value* l,
                                    const char* /*file*/,
                                    int /*line*/)
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

static void dynDestroyFunctionCallback(struct CRYPTO_dynlock_value* l,
                                       const char* /*file*/,
                                       int /*line*/)
{
    delete l;
}

////////////////////////////////////////////////////////////////////////////
/// Check host name.
//////////////

static bool certHostCheck(const std::string &patternStr, const std::string &hostStr)
{
    if (patternStr.empty() || hostStr.empty())
    {
        return false;
    }

    std::string pattern (patternStr);
    std::string host(hostStr);
    
    /* normalize pattern and hostname by stripping off trailing dots */
    if (pattern[pattern.length() - 1] == '.')
    {
        pattern.erase(pattern.length() - 1, 1);
    }

    if (host[host.length() - 1] == '.')
    {
        host.erase(host.length() - 1, 1);
    }

    /* Find wildcard. */
    auto wildcardPos = pattern.find('*');
    if (wildcardPos == std::string::npos)
    {
        /* If not find, just compare the two strings. */
        return parrot::iStringCmp(pattern, host);
    }

    unsigned char buf[sizeof(struct in6_addr)];    
    /*
       Detect IP address as hostname and fail the match if so.
       Common name must be domain name. See:
       https://tools.ietf.org/html/rfc2818#section-3.1
    */    
    if (inet_pton(AF_INET, host.c_str(), buf) == 1)
    {
        return false;
    }
    else if (inet_pton(AF_INET6, host.c_str(), buf) == 1)
    {
        return false;
    }

    /*
       We require at least 2 dots in pattern to avoid too wide wildcard
       match. Also see:
       https://en.wikipedia.org/wiki/Internationalized_domain_name#ASCII_spoofing_concerns
    */
    bool wildcardEnabled  = true;
    auto patternDotPos = pattern.find('.');
    if (patternDotPos == std::string::npos ||
        pattern.find('.', patternDotPos + 1) == std::string::npos || /// At least two dots.
        wildcardPos > patternDotPos || /// Wildcard must be in front of first '.'.
        parrot::iStringCmpN(pattern, "xn--", 4))
    {
        wildcardEnabled = false;
    }

    if (!wildcardEnabled)
    {
        return parrot::iStringCmp(pattern, host);
    }

    // If here, we need to verify wildcard cert.

    auto hostDotPos = host.find('.');
    if (hostDotPos == std::string::npos)
    {
        return false;
    }

    /*
      Both pattern and host must be equal after their first '.'.
     */
    auto pit = pattern.begin();
    auto hit = host.begin();
    for (pit += patternDotPos, hit += hostDotPos;
         pit != pattern.end() || hit != host.end(); ++pit, ++hit)
    {
        if (std::toupper(*pit) != std::toupper(*hit))
        {
            return false;
        }
    }

    /* The wildcard must match at least one character, so the left-most
            label of the hostname is at least as large as the left-most label
            of the pattern. */
    if (hostDotPos < patternDotPos)
    {
        return false;
    }

    /* 
      The pattern can be 'foo*bar.test.com', a host can be fooHellobar.test.com,
      we need to check the substring before '*'. And substring after '*' and
      before first '.'.
     */

    /* Check substring before '*' */
    pit = pattern.begin();
    hit = host.begin();
    for (; pit != pattern.begin() + wildcardPos; ++pit, ++hit)
    {
        if (std::toupper(*pit) != std::toupper(*hit))
        {
            return false;
        }
    }

    /* Check substring after '*' and before first '.' */
    hit = host.begin() + hostDotPos - 1;
    pit = pattern.begin() + patternDotPos - 1;
    for (; pit > pattern.begin() + wildcardPos; --pit, --hit)
    {
        if (std::toupper(*pit) != std::toupper(*hit))
        {
            return false;
        }        
    }

    return true;
}

static bool verifyHost(const std::string &host, const X509* cert)
{
    if (host.empty() || cert == nullptr)
    {
        return false;
    }
    
    bool   matched = false;
    STACK_OF(GENERAL_NAME) * altnames =
        (stack_st_GENERAL_NAME*)X509_get_ext_d2i(
            (X509*)cert, NID_subject_alt_name, nullptr, nullptr);
    int target = GEN_DNS; /// See x509v3.h in openssl project.
    struct in_addr addr4;
    struct in6_addr addr6;
    uint8_t ipver = 4;
    size_t addrlen = 0;
    bool result = true;

    if (inet_pton(AF_INET, host.c_str(), &addr4) == 1)
    {
        addrlen = sizeof(addr4);
        target = GEN_IPADD; /// See x509v3.h in openssl project.
    }
    else if (inet_pton(AF_INET6, host.c_str(), &addr6) == 1)
    {
        addrlen = sizeof(addr6);
        target = GEN_IPADD;
        ipver = 6;
    }

    if (altnames)
    {
        int numalts;
        int i;
        /* get amount of alternatives, RFC2459 claims there MUST be at least
           one, but we don't depend on it... */
        numalts = sk_GENERAL_NAME_num(altnames);

        /* loop through all alternatives while none has matched */
        for (i = 0; (i < numalts) && !matched; i++)
        {
            /* get a handle to alternative name number i */
            const GENERAL_NAME* check = sk_GENERAL_NAME_value(altnames, i);

            /* only check alternatives of the same type the target is */
            if (check->type == target)
            {
                /* get data and length */
                const char* altptr = (char*)ASN1_STRING_data(check->d.ia5);
                size_t      altlen = (size_t)ASN1_STRING_length(check->d.ia5);

                switch (target)
                {
                    case GEN_DNS: /* name/pattern comparison */
                        if ((altlen == strlen(altptr)) &&
                            /* if this isn't true, there was an embedded zero in
                               the name
                                string and we cannot match it. */
                            certHostCheck(altptr, host.c_str()))
                        {
                            matched = true;
                        }
                        break;

                    case GEN_IPADD: /* IP address comparison */
                        /* compare alternative IP address if the data chunk is
                           the same size
                           our server IP address is */
                        if (altlen == addrlen)
                        {
                            if (ipver == 4 && memcmp(altptr, &addr4, altlen) == 0)
                            {
                                matched = true;
                            }
                            else if (ipver == 6 && memcmp(altptr, &addr6, altlen) == 0)
                            {
                                matched = true;
                            }
                        }
                        break;

                    default:
                        break;
                }
            }
        }
        GENERAL_NAMES_free(altnames);
    }

    if (matched)
    {
        // Success.
    }
    else if (altnames)
    {
        result = false;
    }
    else
    {
        /* we have to look to the last occurrence of a commonName in the
           distinguished one to get the most significant one. */
        int j, i = -1;

        /* The following is done because of a bug in 0.9.6b */

        std::string peerCNStr;

        X509_NAME* name = X509_get_subject_name((X509*)cert);
        if (name)
        {
            while ((j = X509_NAME_get_index_by_NID(name, NID_commonName, i)) >=
                   0)
            {
                i = j;
            }
        }

        /* we have the name entry and we will now convert this to a string
           that we can use for comparison. Doing this we support BMPstring,
           UTF8 etc. */

        if (i >= 0)
        {
            ASN1_STRING* tmp =
                X509_NAME_ENTRY_get_data(X509_NAME_get_entry(name, i));
            unsigned char* peer_CN = nullptr;
            
            if (tmp)
            {
                j = ASN1_STRING_to_UTF8(&peer_CN, tmp);

                int cnLen = (int)strlen((char*)peer_CN);
                if (peer_CN && cnLen != j)
                {
                    result = false;
                }
                else
                {
                    peerCNStr = std::string((const char *)peer_CN, cnLen);
                    OPENSSL_free(peer_CN);                                        
                }
            }
        }

        if (!result)
            /* error already detected, pass through */
            ;
        else if (peerCNStr.empty())
        {
            result = false;
        }
        else if (!certHostCheck(peerCNStr, host))
        {
            result = false;
        }
    }

    return result;    
}


// Tries to find a match for hostname in the certificate's Common Name field.
//
// Tries to find a match for hostname in the certificate's
// Subject Alternative Name extension.
static bool matchesCommonName(const std::string& host, const X509* cert)
{
    int              commonNameLoc   = -1;
    X509_NAME_ENTRY* commonNameEntry = nullptr;
    ASN1_STRING*     commonNameAsn1  = nullptr;
    char*            commonNameStr   = nullptr;

    // Find the position of the CN field in the Subject field of the certificate
    commonNameLoc = X509_NAME_get_index_by_NID(
        X509_get_subject_name((X509*)cert), NID_commonName, -1);
    if (commonNameLoc < 0)
    {
        return false;
    }

    // Extract the CN field
    commonNameEntry =
        X509_NAME_get_entry(X509_get_subject_name((X509*)cert), commonNameLoc);
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
    commonNameStr = (char*)ASN1_STRING_data(commonNameAsn1);

    // Make sure there isn't an embedded NUL character in the CN
    if (ASN1_STRING_length(commonNameAsn1) != (int)std::strlen(commonNameStr))
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
static bool matchesSubjectAlternativeName(const std::string& host,
                                          X509*        cert)
{
    int sanNamesNb                   = -1;
    STACK_OF(GENERAL_NAME)* sanNames = nullptr;

    // Try to extract the names within the SAN extension from the certificate
    sanNames = (stack_st_GENERAL_NAME*)X509_get_ext_d2i(
        (X509*)cert, NID_subject_alt_name, nullptr, nullptr);
    if (sanNames == nullptr)
    {
        return false;
    }
    sanNamesNb = sk_GENERAL_NAME_num(sanNames);

    bool res = false;

    // Check each name within the extension
    for (int i = 0; i < sanNamesNb; ++i)
    {
        const GENERAL_NAME* currentName = sk_GENERAL_NAME_value(sanNames, i);

        if (currentName->type == GEN_DNS)
        {
            // Current name is a DNS name, let's check it
            char* dnsName = (char*)ASN1_STRING_data(currentName->d.dNSName);

            // Make sure there isn't an embedded NUL character in the DNS name
            if (ASN1_STRING_length(currentName->d.dNSName) !=
                (int)std::strlen(dnsName))
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
SslHelper::SslHelper()  = default;
SslHelper::~SslHelper() = default;

void SslHelper::init()
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

    verifyHost("", nullptr);
}

void SslHelper::freeThreadErrQueue(const std::thread::id& id)
{
    CRYPTO_THREADID            tid;
    std::hash<std::thread::id> hasher;
    tid.val = (unsigned long)hasher(id);
    ERR_remove_thread_state(&tid);
}

SSL_CTX* SslHelper::genSslCtx(const std::string& keyPath,
                              const std::string& certPath,
                              const std::string& caPath,
                              const std::string& caFile,
                              bool               verifyPeer,
                              int                depth)
{
    const SSL_METHOD* m      = TLSv1_2_method();
    SSL_CTX*          sslCtx = SSL_CTX_new(m);

    if (!sslCtx)
    {
        // If here, did you forget to call SslHelper::init()?
        PARROT_ASSERT(0);
    }

    const char* caFilePtr = nullptr;
    const char* caPathPtr = nullptr;

    if (caFile.length() > 0)
    {
        caFilePtr = caFile.c_str();
    }

    if (caPath.length() > 0)
    {
        caPathPtr = caPath.c_str();
    }

    // Add ca-cert to SSL_CTX.
    if (caFilePtr || caPathPtr)
    {
        // Add ca-cert file.
        if (SSL_CTX_load_verify_locations(sslCtx, caFilePtr, caPathPtr) != 1)
        {
            PARROT_ASSERT(0);
        }
    }

    // Add cert to SSL_CTX.
    if (caPath.length() > 0 &&
        SSL_CTX_use_certificate_file(sslCtx, certPath.c_str(),
                                     SSL_FILETYPE_PEM) <= 0)
    {
        PARROT_ASSERT(0);
    }

    // Add private key to SSL_CTX.
    if (keyPath.length() > 0 &&
        SSL_CTX_use_PrivateKey_file(sslCtx, keyPath.c_str(),
                                    SSL_FILETYPE_PEM) != 1)
    {
        PARROT_ASSERT(0);
    }

    // Enable nonblock.
    SSL_CTX_set_mode(sslCtx, SSL_MODE_ENABLE_PARTIAL_WRITE |
                                 SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

    if (verifyPeer)
    {
        // We need to verify peer, but the client doesn't tell us where
        // to find the ca-certs, we use default.
        if (caPath.length() > 0)
        {
            if (SSL_CTX_set_default_verify_paths(sslCtx) != 1)
            {
                PARROT_ASSERT(0);
            }
        }

        SSL_CTX_set_verify(
            sslCtx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
        SSL_CTX_set_verify_depth(sslCtx, depth);
    }
    else
    {
        SSL_CTX_set_verify(sslCtx, SSL_VERIFY_NONE, nullptr);
    }

    return sslCtx;
}

SSL* SslHelper::genSsl(SSL_CTX* ctx)
{
    SSL* ssl = SSL_new(ctx);
    return ssl;
}

bool SslHelper::checkCertHostname(SSL* ssl, const std::string& host)
{
    X509* cert = nullptr;
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

void SslHelper::deinit()
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
