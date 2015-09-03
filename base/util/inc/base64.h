#ifndef __BASE_UTIL_INC_BASE64_H__
#define __BASE_UTIL_INC_BASE64_H__

#include <cstdint>
#include <vector>

namespace parrot
{
    uint32_t getBase64DecodeLen(const char * bufcoded);
    void base64Decode(std::vector<char> &vecOut, const char * bufcoded);

    int getBase64EncodeLen(int len);
    void base64Encode(std::vector<char> &outVec, const char *in, uint32_t len);
}

#endif
