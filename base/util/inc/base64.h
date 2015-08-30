#ifndef __BASE_UTIL_INC_BASE64_H__
#define __BASE_UTIL_INC_BASE64_H__

#include <string>
#include <vector>

namespace parrot
{
    std::string base64Encode(const std::vecotr<char> &buff);
    std::vector<char> base64Decode(const std::string & s);
}

#endif
