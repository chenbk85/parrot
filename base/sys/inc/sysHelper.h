#ifndef __BASE_SYS_INC_SYSHELPER_H__
#define __BASE_SYS_INC_SYSHELPER_H__

#include <cstdint>

namespace parrot
{
    uint32_t uniHtonl(uint32_t hostlong);

    uint16_t uniHtons(uint16_t hostshort);

    uint32_t uniNtohl(uint32_t netlong);

    uint16_t uniNtohs(uint16_t netshort);
}

#endif
