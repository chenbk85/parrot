#ifndef __BASE_UTIL_INC_BASE64_H__
#define __BASE_UTIL_INC_BASE64_H__

#include <cstdint>

namespace parrot
{
    // getBase64DecodeLen
    //
    // This function computes how long the buffer needed to store the decoded
    // data. The result includes the terminator '\0'.
    //
    // Param:
    // * bufcoded  The base64 string.
    //
    // Return:
    //  The length of original data plus 1.
    uint32_t getBase64DecodeLen(const char * bufcoded);

    // base64Decode
    //
    // This function decodes the base64 string.
    //
    // Param:
    // * bufout   The buffer to store the decoded data.
    // * bufcoded The base64 string.
    //
    // Return:
    //  The length of original data. Doesn't include the terminator '\0'.    
    uint32_t base64Decode(char *bufout, const char * bufcoded);

    // getBase64EncodeLen
    //
    // This function computes how long the buffer needed to store the encoded
    // data. The result includes the terminator '\0'.
    //
    // Param:
    // * len    The length of original data.
    //
    // Return:
    //  The length of encoded data plus 1.    
    uint32_t getBase64EncodeLen(uint32_t len);

    // base64Encode
    //
    // This function encodes the buffer to base64 string.
    //
    // Param:
    // * bufout  The buffer to store the encoded string.
    // * in      The original buffer.
    // * len     The length of original buffer.
    //
    // Return:
    //  The length of encoded string. Doesn't include the ternimator '\0'.
    uint32_t base64Encode(char *bufout, const char *in, uint32_t len);
}

#endif
