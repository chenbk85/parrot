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
        SslHelper(const SslHelper &s) = delete;
        SslHelper& operator=(const SslHelper &s) = delete;
        
      public:
        // init
        // 
        // Register ssl callbacks, init ssl library, ets.
        static void init();

        // freeThreadErrQueue
        // 
        // Remove error queue of thread. When stop a thread, this
        // funciton should be called.
        //
        // Param:
        // * id     The id of the target thread.
        static void freeThreadErrQueue(const std::thread::id &id);

        // genSslCtx
        // 
        // Create a ssl context.
        // 
        // Param:
        // * keyPath     The absolute file path of key file.
        // * certPath    The absolute file path of cert file.
        // * verifyPeer  Verify peer if true.
        // * depth       The verify depth.
        //
        // Return:
        //  A SSL_CTX pointer.
        static SSL_CTX* genSslCtx(const std::string &keyPath, 
                                  const std::string &certPath, 
                                  bool verifyPeer = false,
                                  int depth = 1);

        // genSsl
        // 
        // Create a ssl object by ssl context.
        //
        // Param:
        // * ctx     SSL_CTX.
        //
        // Return:
        //  A SSL_CTX pointer.
        static SSL* genSsl(SSL_CTX *ctx);

        // checkCertHostname
        // 
        // For server, if you want to verify client cert, call this 
        // function after accept. For client to verify server, call
        // this function after connect.
        // 
        // Param:
        // * ssl         The ssl of the connection.       
        // * host        The name of host.
        //
        // Return:
        //   True if successful.
        static bool checkCertHostname(SSL *ssl, const std::string &host);

        // deinit
        //
        // Free ssl memeory.
        static void deinit();
    };
}

#endif
