#ifndef __BASE_SYS_INC_SSLIO_H__
#define __BASE_SYS_INC_SSLIO_H__

namespace parrot
{
    enum class SslIoStatus
    {
        Ok,
        RetryAtWriteFd,
        RetryAtReadFd,
        Error
    };

    class SslIo : public IoEvent
    {
      public:
        SslIo();
        ~SslIo();
        SslIo(const SslIo&) = delete;
        SslIo & operator=(const SslIo&) = delete;

      public:
        // setSsl
        //
        // For client, call this function after successfully connecting 
        // to remote server. Then, call doSslConnect to finish ssl handshake.
        // For server, call this function after successfully accepting a 
        // connection. Then, call doSslAccept to finish ssl handshake.
        // 
        // Param:
        // * ssl    The ssl pointer.
        void setSsl(SSL *ssl);

        // doSslConnect
        //
        // After successfully connecting to server, call this function to
        // do ssl handshake.
        //
        // Return:
        //  The status of handshake.
        SslIoStatus doSslConnect();

        // doSslAccept
        //
        // After successfully accepting a connection, call this function to
        // do ssl handshake.
        //
        // Return:
        //  The status of handshake.
        SslIoStatus doSslAccept();

        // sslSend
        //
        // Send buffer to peer.
        //
        // Param:
        // * buff     [in]      The buffer address.
        // * len      [in]      The length of buffer.
        // * sentLen  [out]     The bytes has been sent.
        //
        // Return:
        //  The status of sslSend.
        SslIoStatus sslSend(const char *buff, uint32_t len, uint32_t &sentLen);

        // sslRecv
        //
        // Read data from connection.
        //
        // Param:
        // * buff     [out]     The buffer address.
        // * len      [in]      The length of buffer.
        // * recvLen  [out]     The bytes has been read.
        //
        // Return:
        //  The status of sslRecv.
        SslIoStatus sslRecv(char *buff, uint32_t len, uint32_t &recvLen);

        // closeSsl
        //
        // Free ssl memory.
        void closeSsl();

      protected:
        SslIoStatus handleResult(int ret, const string &funcName);

      protected:
        SSL * _ssl;
    }
}

#endif
