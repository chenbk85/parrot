#include <algorithm>
#include <locale>

#include "stringHelper.h"

namespace parrot
{
bool iStringCmp(const std::string& s1, const std::string& s2)
{
    if (s1.length() != s2.length())
    {
        return false;
    }

    for (auto it1 = s1.begin(), it2 = s2.begin();
         it1 != s1.end() || it2 != s2.end(); ++it1, ++it2)
    {
        if (std::tolower(*it1) != std::tolower(*it2))
        {
            return false;
        }
    }

    return true;
}

bool iStringCmpN(const std::string& s1, const std::string& s2, size_t len)
{
    for (auto it1 = s1.begin(), it2 = s2.begin();
         (it1 != s1.end() || it2 != s2.end()) && len > 0;
         ++it1, ++it2, --len)
    {
        if (std::tolower(*it1) != std::tolower(*it2))
        {
            return false;
        }
    }

    if (len == 0)
    {
        return true;
    }

    return false;
}

void strToLower(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

void strToUpper(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
}

size_t iStringFind(const std::string& src, const std::string& target)
{
    auto it = std::search(src.begin(), src.end(), target.begin(), target.end(),
                          [](char ch1, char ch2)
                          {
                              return std::toupper(ch1) == std::toupper(ch2);
                          });

    return it == src.end() ? std::string::npos : it - src.begin();
}

void binToHexLowCase(unsigned char* bin, uint32_t binLen, char* result)
{
    const char* hexStr = "0123456789abcdef";

    result[binLen << 1] = 0;
    for (uint32_t i = 0; i < binLen; ++i)
    {
        result[(i << 1) + 0] = hexStr[(bin[i] >> 4) & 0x0F];
        result[(i << 1) + 1] = hexStr[(bin[i]) & 0x0F];
    }
}

void binToHexUpCase(unsigned char* bin, uint32_t binLen, char* result)
{
    const char* hexStr = "0123456789ABCDEF";

    result[binLen << 1] = 0;
    for (uint32_t i = 0; i < binLen; ++i)
    {
        result[(i << 1) + 0] = hexStr[(bin[i] >> 4) & 0x0F];
        result[(i << 1) + 1] = hexStr[(bin[i]) & 0x0F];
    }
}
}
