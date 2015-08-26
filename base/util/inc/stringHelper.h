#ifndef __BASE_UTIL_INC_STRINGHELPER_H__
#define __BASE_UTIL_INC_STRINGHELPER_H__

#include <string>

namespace parrot
{
    // iStringCmp
    // 
    // Compare string case insensitively. E.g., Compare "Apple" and "aPPle" 
    // will return true. "Apple" and "AppleA" will return false.
    // 
    // Param:
    // * s1    A string.
    // * s2    A string.
    //
    // Return:
    //  True if case insensitively equal. 
    bool iStringCmp(const std::string &s1, const std::string &s2);
}

#endif
