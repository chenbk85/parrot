#include <openssl/err.h>

#include "sslIo.h"
#include "logger.h"

namespace parrot
{
    SslIo::SslIo():
        IoEvent(),
        _ssl(nullptr)
    {
    }
    
    SslIo::~SslIo()
    {
        closeSsl();
    }

    void SslIo::setSsl(SSL *ssl)
    {
        _ssl = ssl;
        BIO *bio = BIO_new_socket(getFd(), BIO_NOCLOSE);
        SSL_set_bio(_ssl, bio, bio);
    }

    SslIoStatus SslIo::doSslConnect()
    {
        int ret = SSL_connect(_ssl);
        return handleResult(ret, "doSslConnect");
    }

    SslIoStatus SslIo::doSslAccept()
    {
        int ret = SSL_accept(_ssl);
        return handleResult(ret, "doSslAccept");
    }

    SslIoStatus SslIo::sslSend(const char *buff, uint32_t len, 
                               uint32_t &sentLen)
    {
        int wLen = SSL_write(_ssl, buff, (int)len);

        if (wLen > 0) 
        {
            sentLen = wLen;
        }

        return handleResult(wLen, "sslSend");
    }

    SslIoStatus SslIo::sslRecv(char *buff, uint32_t len, 
                                      uint32_t &recvLen)
    {
        int rLen = SSL_read(_ssl, buff, (int)len);

        if (wLen > 0)
        {
            recvLen = rLen;
        }

        return handleResult(wLen, "sslRecv");
    }

    SslIoStatus SslIo::handleResult(int ret, const string &funcName)
    {
        SslIoStatus status = SslIoStatus::Error;

        if (ret > 0) 
        {
            status = SslIoStatus::Ok;
        }
        else if (ret == 0)
        {
            char errBuf[512];
            int err = SSL_get_error(_ssl, wLen);
            ERR_error_string_n(err, errBuf, sizeof(errBuf));
            LOG_ERROR("SslIo::handleResult: " << funcName << ".  Ret is " 
                      << ret << ". Err is " << errbuf << ".");
        }
        else
        {
            int err = SSL_get_error(_ssl, ret);

            switch (err)
            {
                case SSL_ERROR_WANT_READ:
                    status = SslIoStatus::RetryAtReadFd;
                    break;

                case SSL_ERROR_WANT_WRITE:
                    status = SslIoStatus::RetryAtWriteFd;
                    break;
                
                default:
                    char errBuf[512];
                    ERR_error_string_n(err, errBuf, sizeof(errBuf));
                    LOG_ERROR("SslIo::handleResult: " << funcName 
                              << ".  Ret is " << ret << ". Err is " 
                              << errbuf << ".");
                    break;
            }
        }

        return status;
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
