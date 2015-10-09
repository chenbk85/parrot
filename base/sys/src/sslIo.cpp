#include <openssl/err.h>

#include "ioEvent.h"
#include "sslIo.h"
#include "logger.h"

namespace parrot
{
SslIo::SslIo() : IoEvent(), _ssl(nullptr)
{
}

SslIo::~SslIo()
{
    closeSsl();
}

void SslIo::setSsl(SSL* ssl)
{
    _ssl = ssl;
    BIO* bio = BIO_new_socket(getFd(), BIO_NOCLOSE);
    SSL_set_bio(_ssl, bio, bio);
}

eCodes SslIo::doSslConnect()
{
    int ret = SSL_connect(_ssl);
    return handleResult(ret, "doSslConnect");
}

eCodes SslIo::doSslAccept()
{
    int ret = SSL_accept(_ssl);
    return handleResult(ret, "doSslAccept");
}

eCodes SslIo::send(const char* buff, uint32_t len, uint32_t& sentLen)
{
    int wLen = SSL_write(_ssl, buff, (int)len);

    if (wLen > 0)
    {
        sentLen = wLen;
    }

    return handleResult(wLen, "sslSend");
}

eCodes SslIo::recv(char* buff, uint32_t len, uint32_t& recvLen)
{
    int rLen = SSL_read(_ssl, buff, (int)len);

    if (rLen > 0)
    {
        recvLen = rLen;
    }

    return handleResult(rLen, "sslRecv");
}

eCodes SslIo::handleResult(int ret, const std::string& funcName)
{
    eCodes code = eCodes::ERR_Fail;

    if (ret > 0)
    {
        code = eCodes::ST_Ok;
    }
    else if (ret == 0)
    {
        char errBuf[512];
        int err = SSL_get_error(_ssl, ret);
        ERR_error_string_n(err, errBuf, sizeof(errBuf));
        LOG_ERROR("SslIo::handleResult: " << funcName << ".  Ret is " << ret
                                          << ". Err is " << errBuf << ".");
    }
    else
    {
        int err = SSL_get_error(_ssl, ret);

        switch (err)
        {
        case SSL_ERROR_WANT_READ:
            code = eCodes::ST_RetryWhenReadable;
            break;

        case SSL_ERROR_WANT_WRITE:
            code = eCodes::ST_RetryWhenWritable;
            break;

        default:
            char errBuf[512];
            ERR_error_string_n(err, errBuf, sizeof(errBuf));
            LOG_ERROR("SslIo::handleResult: " << funcName << ".  Ret is " << ret
                                              << ". Err is " << errBuf << ".");
            break;
        }
    }

    return code;
}

void SslIo::closeSsl()
{
    if (_ssl)
    {
        SSL_free(_ssl);
        _ssl = nullptr;
    }
}
}
