#include "stringHelper.h"
#include <locale>

namespace parrot
{
    bool iStringCmp(const std::string &s1, const std::string &s2)
    {
        if (s1.length() != s2.length())
        {
            return false;
        }

        for (auto it1 = s1.begin(), it2 = s2.begin(); 
             it1 != s1.end(); ++it1, ++it2)
        {
            if (std::tolower(*it1) != std::tolower(*it2))
            {
                return false;
            }
        }

        return true;
    }
}
