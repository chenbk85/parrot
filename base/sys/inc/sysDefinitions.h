#ifndef __BASE_SYS_INC_SYSDEFINITIONS_H__
#define __BASE_SYS_INC_SYSDEFINITIONS_H__

#include <cstdint>

namespace parrot
{
enum class eIoAction : uint8_t
{
    None,
    Read,
    Write,
    ReadWrite,
    Remove
};


}

#endif
