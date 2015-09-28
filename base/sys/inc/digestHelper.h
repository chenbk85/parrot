#ifndef __BASE_SYS_INC_DIGESTHELPER_H__
#define __BASE_SYS_INC_DIGESTHELPER_H__

#include <cstdint>
#include "codes.h"

namespace parrot
{
// sha1Message
//
// Compute the sha1 digest of the message.
//
// Param:
// * buff   [IN ] The binary buffer.
// * binLen [IN ] The length of binary buffer.
// * result [OUT] The sha1 binary data.
eCodes sha1Message(unsigned char* buff, uint32_t buffLen, unsigned char* out);
}

#endif
