#if defined(__linux__)
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#elif defined(__APPLE__)
#include <unistd.h>
#include <fcntl.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#elif defined(_WIN32)
#include <Winsock2.h>
#endif

#include <system_error>

#include "sysHelper.h"

namespace parrot
{
uint64_t uniHtonll(uint64_t x)
{
    return ((1 == htonl(1)) ? (x)
                              : ((uint64_t)htonl((x)&0xFFFFFFFF) << 32) |
                                    htonl((x) >> 32));
}

uint32_t uniHtonl(uint32_t x)
{
    return htonl(x);
}

uint16_t uniHtons(uint16_t x)
{
    return htons(x);
}

uint64_t uniNtohll(uint64_t x)
{
    return ((1 == ntohl(1)) ? (x)
                              : ((uint64_t)ntohl((x)&0xFFFFFFFF) << 32) |
                                    ntohl((x) >> 32));
}

uint32_t uniNtohl(uint32_t x)
{
    return ntohl(x);
}

uint16_t uniNtohs(uint16_t x)
{
    return ntohs(x);
}

#if defined(_WIN32)
void setExclusiveAddr(sockhdl fd)
{
    BOOL optVal = TRUE;
    int optLen = sizeof(BOOL);

    int ret =
        setsockopt(fd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&optVal, optLen);
    if (ret != 0)
    {
        throw std::system_error(WSAGetLastError(), std::system_category(),
                                "WinsetExclusiveAddr");
    }
}
#else
void setReuseAddr(sockhdl fd)
{
    int optVal = 1;
    socklen_t optLen = sizeof(optVal);

    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*)&optVal, optLen);
    if (ret != 0)
    {
        throw std::system_error(errno, std::system_category(),
                                "setNoDelay");
    }
}
#endif

void setNonBlock(sockhdl fd, bool on)
{
#if defined(__APPLE__) || defined(__linux__)
    int oldFlags = ::fcntl(fd, F_GETFL, 0);
    if (oldFlags < 0)
    {
        throw std::system_error(errno, std::system_category(),
                                "setNonBlock: Get");
    }

    int newFlags = 0;
    if (on)
    {
        newFlags = oldFlags | O_NONBLOCK;
    }
    else
    {
        newFlags = oldFlags | ~O_NONBLOCK;
    }

    if (::fcntl(fd, F_SETFL, newFlags) < 0)
    {
        throw std::system_error(errno, std::system_category(),
                                "setNonBlock: Set");
    }
#elif defined(_WIN32)
    u_long mode = 1;
    if (!on)
    {
        mode = 0;
    }

    if (ioctlsocket(fd, FIONBIO, &mode) != 0)
    {
        throw std::system_error(WSAGetLastError(), std::system_category(),
                                "setNonBlock");
    }
#endif
}

void setNoDelay(sockhdl fd)
{
#if defined(__APPLE__) || defined(__linux__)
    int optVal = 1;
    socklen_t optLen = sizeof(optVal);

    int ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&optVal, optLen);
    if (ret != 0)
    {
        throw std::system_error(errno, std::system_category(),
                                "setNoDelay");
    }
#elif defined(_WIN32)
    BOOL optVal = TRUE;
    int optLen = sizeof(optVal);

    int ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&optVal, optLen);
    if (ret != 0)
    {
        throw std::system_error(WSAGetLastError(), std::system_category(),
                                "setNoDelay");
    }
#endif
}
}
