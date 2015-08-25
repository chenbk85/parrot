#ifndef __BASE_SECURITY_INC_SECURITY_H__
#define __BASE_SECURITY_INC_SECURITY_H__

namespace parrot
{
    class Security
    {
      public:
        Security();
        ~Security();
        Security(const Security &s) = delete;
        Security& operator=(const Security &s) = delete;
        
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
        // Create a ssl contex.
        // 
        // Param:
        // * keyPath     The absolute file path of key file.
        // * certPath    The absolute file path of cert file.
        // * verifyPeer  Verify peer if true.
        // * depth       The verify depth.
        static ssl_ctx* genSslCtx(const string &keyPath, 
                                  const string &certPath, 
                                  bool verifyPeer = false,
                                  int depth = 1);

        // checkConnection
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
        static bool checkConnection(SSL *ssl, const string &host);

        // deinit
        //
        // Free ssl memeory.
        static void deinit();
    };
}

#endif
