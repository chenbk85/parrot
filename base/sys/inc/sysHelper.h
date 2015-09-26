#ifndef __BASE_SYS_INC_SYSHELPER_H__
#define __BASE_SYS_INC_SYSHELPER_H__

#include <cstdint>

namespace parrot {
uint64_t uniHtonll(uint64_t hostll);

uint32_t uniHtonl(uint32_t hostlong);

uint16_t uniHtons(uint16_t hostshort);

uint64_t uniNtohll(uint64_t netll);

uint32_t uniNtohl(uint32_t netlong);

uint16_t uniNtohs(uint16_t netshort);
}

#endif
