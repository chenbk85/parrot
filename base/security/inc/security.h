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
        static void init();
        static void freeThreadErrQueue(const std::thread::id &id);
        static void deinit();
        static ssl_ctx* genSslCtx(const string &keyPath, 
                                  const string &certPath, 
                                  bool verifyPeer = false);

        
    };
}

#endif
