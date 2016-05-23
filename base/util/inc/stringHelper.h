#ifndef __BASE_UTIL_INC_STRINGHELPER_H__
#define __BASE_UTIL_INC_STRINGHELPER_H__

#include <string>

namespace parrot
{

/**
 * Compare string case insensitively. E.g., Compare "Apple" and "aPPle"
 * will return true. "Apple" and "AppleA" will return false.
 *
 * @param   s1        A string.
 * @param   s2        A string.
 * @return            True if case insensitively equal.
 */
bool iStringCmp(const std::string& s1, const std::string& s2);

/**
 * Compare string case insensitively with specified length.
 *
 * @param   s1        A string.
 * @param   s2        A string.
 * @param   len       The length will be compared.
 * @return            True if case insensitively equal.
 */
bool iStringCmpN(const std::string& s1, const std::string& s2, size_t len);

/**
 * Conver string to lower case. E.g., "aAaaBB" -> "aaaabb"
 *
 * @param str  The string that will be converted.
 */
void strToLower(std::string& str);

/**
 * Conver string to uppper case. E.g., "aAaaBB" -> "AAAABB".
 *
 * @param str  The string that will be converted.
 */
void strToUpper(std::string& str);

/**
 * Case insensitive std::string::find().
 *
 * @param  src    The string that will be searched.
 * @param  target The string that needs to be found.
 *
 * @return        If found, returnt the index of target of src,
 *                else return std::string::npos
 */
size_t iStringFind(const std::string& src, const std::string& target);

/**
 * Binary to lower case hex string.
 *
 * @param  bin     The binary buffer.
 * @param  binLen  The length of binary buffer.
 * @param  result  The hex string result.
 */
void binToHexLowCase(unsigned char* bin, uint32_t binLen, char* result);

/**
 * Binary to upper case hex string.
 *
 * @param  bin     The binary buffer.
 * @param  binLen  The length of binary buffer.
 * @param  result  The hex string result.
 */
void binToHexUpCase(unsigned char* bin, uint32_t binLen, char* result);
}

#endif
