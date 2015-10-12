#ifndef __BASE_SYS_INC_SSLIO_H__
#define __BASE_SYS_INC_SSLIO_H__

#include <openssl/ssl.h>
#include <string>
#include <cstdint>

#include "codes.h"
#include "ioEvent.h"

namespace parrot
{
class SslIo : public IoEvent
{
  public:
    SslIo();
    ~SslIo();
    SslIo(const SslIo&) = delete;
    SslIo& operator=(const SslIo&) = delete;

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
    void setSsl(SSL* ssl);

    // doSslConnect
    //
    // After successfully connecting to server, call this function to
    // do ssl handshake.
    //
    // Return:
    //  ST_RetryWhenReadable
    //  ST_RetryWhenWriteable
    //  ST_Ok
    //  ERR_Fail
    eCodes doSslConnect();

    // doSslAccept
    //
    // After successfully accepting a connection, call this function to
    // do ssl handshake.
    //
    // Return:
    //  ST_RetryWhenReadable
    //  ST_RetryWhenWriteable
    //  ST_Ok
    //  ERR_Fail
    eCodes doSslAccept();

    // send
    //
    // Send buffer to peer.
    //
    // Param:
    // * buff     [in]      The buffer address.
    // * len      [in]      The length of buffer.
    // * sentLen  [out]     The bytes has been sent.
    //
    // Return:
    //  ST_RetryWhenReadable
    //  ST_RetryWhenWriteable
    //  ST_Ok
    //  ERR_Fail
    eCodes send(const char* buff, uint32_t len, uint32_t& sentLen) override;

    // recv
    //
    // Read data from connection.
    //
    // Param:
    // * buff     [out]     The buffer address.
    // * len      [in]      The length of buffer.
    // * recvLen  [out]     The bytes has been read.
    //
    // Return:
    //  ST_RetryWhenReadable
    //  ST_RetryWhenWriteable
    //  ST_Ok
    //  ERR_Fail
    eCodes recv(char* buff, uint32_t len, uint32_t& recvLen) override;

    // closeSsl
    //
    // Free ssl memory.
    void closeSsl();

  protected:
    // handshake
    //
    // Process the return value of ssl operations.
    //
    // Param:
    // * ret         The return value.
    // * function    The name of the caller function.
    //
    // Return:
    //  ST_RetryWhenReadable
    //  ST_RetryWhenWriteable
    //  ST_Ok
    //  ERR_Fail
    eCodes handleResult(int ret, const char* funcName);

  protected:
    SSL* _ssl;
};
}

#endif
