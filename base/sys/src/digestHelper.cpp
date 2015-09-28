#include <openssl/evp.h>
#include <memory>
#include "digestHelper.h"

namespace parrot
{
eCodes sha1Message(unsigned char* buff, uint32_t buffLen, unsigned char* out)
{
    auto delFunc = [](EVP_MD_CTX* ctxPtr)
    {
        EVP_MD_CTX_destroy(ctxPtr);
    };

    std::unique_ptr<EVP_MD_CTX, decltype(delFunc)> ctx(EVP_MD_CTX_create(),
                                                       delFunc);

    if (!ctx.get())
    {
        return eCodes::ERR_Fail;
    }

    if (1 != EVP_DigestInit_ex(ctx.get(), EVP_sha1(), nullptr))
    {
        return eCodes::ERR_Fail;
    }

    if (1 != EVP_DigestUpdate(ctx.get(), buff, buffLen))
    {
        return eCodes::ERR_Fail;
    }

    uint32_t outLen = 0;
    if (1 != EVP_DigestFinal_ex(ctx.get(), out, &outLen))
    {
        return eCodes::ERR_Fail;
    }

    return eCodes::ST_Ok;
}
}
