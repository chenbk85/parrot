#include "sysHelper.h"

#if defined(__linux__) || defined(__APPLE__)
#include <arpa/inet.h>
#elif defined(_WIN32)
#include <Winsock2.h>
#endif

namespace parrot
{
uint64_t uniHtonll(uint64_t x)
{
    return ((1 == ::htonl(1)) ? (x)
                              : ((uint64_t)::htonl((x)&0xFFFFFFFF) << 32) |
                                    ::htonl((x) >> 32));
}

uint32_t uniHtonl(uint32_t x)
{
    return ::htonl(x);
}

uint16_t uniHtons(uint16_t x)
{
    return ::htons(x);
}

uint64_t uniNtohll(uint64_t x)
{
    return ((1 == ::ntohl(1)) ? (x)
                              : ((uint64_t)::ntohl((x)&0xFFFFFFFF) << 32) |
                                    ::ntohl((x) >> 32));
}

uint32_t uniNtohl(uint32_t x)
{
    return ::ntohl(x);
}

uint16_t uniNtohs(uint16_t x)
{
    return ::ntohs(x);
}
}
