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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "macroFuncs.h"
#include "mtRandom.h"
#include "sslHelper.h"
#include "stringHelper.h"

////////////////////////////////////////////////////////////////////////////
/// Check host name.
//////////////

static bool certHostCheck(const std::string& patternStr,
                          const std::string& hostStr)
{
    if (patternStr.empty() || hostStr.empty())
    {
        return false;
    }

    std::string pattern(patternStr);
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

    /*
       We require at least 2 dots in pattern to avoid too wide wildcard
       match. Also see:
       https://en.wikipedia.org/wiki/Internationalized_domain_name#ASCII_spoofing_concerns
    */
    bool wildcardEnabled = true;
    auto patternDotPos   = pattern.find('.');
    if (patternDotPos == std::string::npos ||
        pattern.find('.', patternDotPos + 1) ==
            std::string::npos || /// At least two dots.
        wildcardPos >
            patternDotPos || /// Wildcard must be in front of first '.'.
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

static bool verifyHost(const std::string& host, const X509* cert)
{
    if (host.empty() || cert == nullptr)
    {
        return false;
    }

    bool matched                     = false;
    STACK_OF(GENERAL_NAME)* altnames = (stack_st_GENERAL_NAME*)X509_get_ext_d2i(
        (X509*)cert, NID_subject_alt_name, nullptr, nullptr);
    int             target = GEN_DNS; /// See x509v3.h in openssl project.
    struct in_addr  addr4;
    struct in6_addr addr6;
    uint8_t         ipver   = 4;
    size_t          addrlen = 0;
    bool            result  = true;

    if (inet_pton(AF_INET, host.c_str(), &addr4) == 1)
    {
        addrlen = sizeof(addr4);
        target  = GEN_IPADD; /// See x509v3.h in openssl project.
    }
    else if (inet_pton(AF_INET6, host.c_str(), &addr6) == 1)
    {
        addrlen = sizeof(addr6);
        target  = GEN_IPADD;
        ipver   = 6;
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
                            if (ipver == 4 &&
                                memcmp(altptr, &addr4, altlen) == 0)
                            {
                                matched = true;
                            }
                            else if (ipver == 6 &&
                                     memcmp(altptr, &addr6, altlen) == 0)
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

        /* If here, we have to look to the last occurrence of a commonName
           in the distinguished one to get the most significant one. */

        /* Common name must be domain name. See:
           https://tools.ietf.org/html/rfc2818#section-3.1
        */
        if (target == GEN_IPADD)
        {
            return false;
        }

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
                    peerCNStr = std::string((const char*)peer_CN, cnLen);
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

//////////////////////////////////////////////////////////////////////////////
namespace parrot
{
SslHelper::SslHelper()  = default;
SslHelper::~SslHelper() = default;

void SslHelper::init()
{
}

SSL_CTX* SslHelper::genSslCtx(const std::string& keyPath,
                              const std::string& certPath,
                              const std::string& caPath,
                              const std::string& caFile,
                              bool               verifyPeer,
                              int                depth)
{
    /* This method supprots from SSLv3 to newest TLS. SSLv3 will be disabled
     * below. */
    const SSL_METHOD* m      = TLS_method();
    SSL_CTX*          sslCtx = SSL_CTX_new(m);

    if (!sslCtx)
    {
        /* If here, did you forget to call SslHelper::init()? */
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

    /* Add ca-cert to SSL_CTX. */
    if (caFilePtr || caPathPtr)
    {
        /* Add ca-cert file. */
        if (SSL_CTX_load_verify_locations(sslCtx, caFilePtr, caPathPtr) != 1)
        {
            PARROT_ASSERT(0);
        }
    }

    /* Add cert to SSL_CTX. */
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
        /* We need to verify peer, but the client doesn't tell us where
         * to find the ca-certs, we use default. */
        if (caPath.empty())
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

    /* TLSv1.0 is the minimal requirement. SSLv3.0 will not be supported. */
    PARROT_ASSERT(SSL_CTX_set_min_proto_version(sslCtx, TLS1_VERSION) == 1);

    return sslCtx;
}

void SslHelper::enableSslSessionCache(SSL_CTX* ctx)
{
    unsigned char        sidCtx[16]{};
    const unsigned char* ln = reinterpret_cast<const unsigned char*>(
        "0123456789abcdefghijklmnopqrstuvwxyz");
    MtRandom r;

    for (auto i = 0u; i < sizeof(sidCtx) - 1; ++i)
    {
        sidCtx[i] = ln[r.random(36)]; // 10 numbers + 26 letters.
    }

    SSL_CTX_set_session_id_context(ctx, sidCtx, sizeof(sidCtx));
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

    if (!verifyHost(host, cert))
    {
        return false;
    }

    X509_free(cert);
    return true;
}

void SslHelper::deinit()
{
}
}
