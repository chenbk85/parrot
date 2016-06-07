#ifndef __BASE_SECURITY_INC_SSLHELPER_H__
#define __BASE_SECURITY_INC_SSLHELPER_H__

#include <string>
#include <thread>

#include "openssl/ssl.h"

namespace parrot
{
class SslHelper
{
  public:
    SslHelper();
    ~SslHelper();
    SslHelper(const SslHelper& s) = delete;
    SslHelper& operator=(const SslHelper& s) = delete;

  public:
    /**
     * Init ssl libray. I start to use the new version openssl. It is simpler
     * then the older. No init, deinit and setup lock callback is required 
     * anymore.
     */
    static void init();

    /**
     * Create a ssl context.
     *
     * @param   keyPath     The absolute file path of key file.
     * @param   certPath    The absolute file path of cert file.
     * @param   caPath      The absolute path of folder of ca-cert file.
     * @param   caFile      The absolute path of ca-cert file.
     * @param   verifyPeer  Verify peer if true.
     * @param   depth       The verify depth.
     *
     * @return A SSL_CTX pointer.
     */
    static SSL_CTX* genSslCtx(const std::string& keyPath = "",
                              const std::string& certPath = "",
                              const std::string& caPath = "",
                              const std::string& caFile = "",
                              bool verifyPeer = false,
                              int depth = 1);

    /**
     * Create a ssl object by SSL_CTX.
     *
     * @param ctx     An object of SSL_CTX.
     * @return A ssl object.
     */
    static SSL* genSsl(SSL_CTX* ctx);

    /**
     * Enable ssl session at the server side.
     *
     * @param ctx     An object of SSL_CTX.
     */
    static void enableSslSessionCache(SSL_CTX* ctx);

    /**
     * Verify remote.
     *
     * @param ssl   SSL object.
     * @param host  The remote host (domain name or ip address).
     * 
     * @return  True if successful. Otherwise false.
     */
    static bool checkCertHostname(SSL* ssl, const std::string& host);


    /**
     * Free memory.
     */
    static void deinit();
};
}

#endif
