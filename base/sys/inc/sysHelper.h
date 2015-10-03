#ifndef __BASE_SYS_INC_SYSHELPER_H__
#define __BASE_SYS_INC_SYSHELPER_H__

#include <cstdint>
#include "unifyPlatDef.h"

namespace parrot
{
uint64_t uniHtonll(uint64_t hostll);

uint32_t uniHtonl(uint32_t hostlong);

uint16_t uniHtons(uint16_t hostshort);

uint64_t uniNtohll(uint64_t netll);

uint32_t uniNtohl(uint32_t netlong);

uint16_t uniNtohs(uint16_t netshort);

#if defined(_WIN32)
// Set the socket exclusive.
//
// Params:
// * fd: The target file descriptor.
void setExclusiveAddr(sockhdl fd);
#elif defined(__linux__) || defined(__APPLE__)
// Do not use reuse addr in Windows.
//
// Params:
// * fd: The target file descriptor.
void setReuseAddr(sockhdl fd);
#endif

// help functions.

// Make the fd non-blocking.
//
// Params:
// * fd: The target file descriptor.
// * on: If true, make the fd nonblock. or make it block.
void setNonBlock(sockhdl fd, bool on = true);

// Turn off nagle algorithm.
//
// Params:
// * fd: The target file descriptor.
void setNoDelay(sockhdl fd);
}

#endif
