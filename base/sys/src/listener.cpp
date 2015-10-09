#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <netinet/in.h>

#include "ioEvent.h"
#include "listener.h"
#include "ipHelper.h"
#include "sysHelper.h"
#include "macroFuncs.h"

// struct sockaddr
// {
//     uint8_t sa_len;
//     sa_family_t sa_family; /* address family: AF_xxx value */
//     char sa_data[14];      /* protocol-specific address */
// };

// struct in_addr
// {
//     in_addr_t s_addr; /* 32-bit IPv4 address */
//                       /* network byte ordered */
// };

// struct sockaddr_in
// {
//     uint8_t sin_len;         /* length of structure (16) */
//     sa_family_t sin_family;  /* AF_INET */
//     in_port_t sin_port;      /* 16-bit TCP or UDP port number */
//                              /* network byte ordered */
//     struct in_addr sin_addr; /* 32-bit IPv4 address */
//                              /* network byte ordered */
//     char sin_zero[8];        /* unused */
// };

// struct in6_addr
// {
//     uint8_t s6_addr[16]; /* 128-bit IPv6 address */
//                          /* network byte ordered */
// };

// struct sockaddr_in6
// {
//     uint8_t sin6_len;          /* length of this struct (28) */
//     sa_family_t sin6_family;   /* AF_INET6 */
//     in_port_t sin6_port;       /* transport layer port# */
//                                /* network byte ordered */
//     uint32_t sin6_flowinfo;    /* flow information, undefined */
//     struct in6_addr sin6_addr; /* IPv6 address */
//                                /* network byte ordered */
//     uint32_t sin6_scope_id;    /* set of interfaces for a scope */
// };

namespace parrot
{
Listener::Listener(uint16_t listenPort, const std::string& listenIp)
    : _listenIp(listenIp), _listenPort(listenPort)
{
    PARROT_ASSERT(_listenPort != 0);
}

eIoAction Listener::handleIoEvent()
{
    return eIoAction::Read;
}

void Listener::startListen()
{
    socklen_t addrLen;
    struct sockaddr* addr;
    struct sockaddr_in addr4;
    struct sockaddr_in6 addr6;

    int fd = 0;

    if (_listenIp.isIPv6())
    {
        std::memset(&addr6, 0, sizeof(struct sockaddr_in6));
        addrLen = sizeof(struct sockaddr_in6);
        addr = (struct sockaddr*)&addr6;
        addr6.sin6_family = AF_INET6;
        addr6.sin6_port = uniHtons(_listenPort);

        auto addr6Bin = _listenIp.getIPv6Bin();
        std::memcpy(&addr6.sin6_addr, &addr6Bin, addrLen);

        // Create socket for IPv6.
        fd = ::socket(AF_INET6, SOCK_STREAM, 0);
    }
    else
    {
        std::memset(&addr4, 0, sizeof(struct sockaddr_in));
        addrLen = sizeof(struct sockaddr_in);
        addr = (struct sockaddr*)&addr4;
        addr4.sin_family = AF_INET;
        addr4.sin_port = uniHtons(_listenPort);
        addr4.sin_addr.s_addr = _listenIp.getIPv4Bin().s_addr;

        // Create socket for IPv4
        fd = ::socket(AF_INET, SOCK_STREAM, 0);
    }

    if (fd == -1)
    {
        throw std::system_error(errno, std::system_category(),
                                "Listener::startListen: create socket");
    }

    int ret = 0;

    try
    {
        // Set reuse addr before bind. Or it won't work.
        // On linux or mac, we can reuse addr. but on windows, we cannot.
        setReuseAddr(fd);

        // Close fd when any exec-family functions succeeded.
        ret = ::fcntl(fd, F_SETFD, FD_CLOEXEC);
        if (ret == -1)
        {
            throw std::system_error(errno, std::system_category(),
                                    "Listener::startListen: fcntl");
        }

        // Bind.
        ret = ::bind(fd, addr, addrLen);
        if (ret == -1)
        {
            throw std::system_error(errno, std::system_category(),
                                    "Listener::startListen: bind");            
        }

        // Listen.
        ret = ::listen(fd, 10000);
        if (ret == -1)
        {
            throw std::system_error(errno, std::system_category(),
                                    "Listener::startListen: listen");
        }

        // Enable non blocking.
        setNonBlock(fd, true);

        // Disable Nagle's algorithm.
        setNoDelay(fd);

        // Finally set the fd.
        setFd(fd);
    }
    catch (...)
    {
        if (fd != -1)
        {
            ::close(fd);
        }
        throw;
    }

    setAction(eIoAction::Read);
}

eCodes Listener::doAccept(int& cliFd, IPHelper& cliIP, uint16_t& cliPort)
{
    struct sockaddr_in6 cliAddr6;
    struct sockaddr* cliAddr = reinterpret_cast<struct sockaddr*>(&cliAddr6);
    socklen_t cliAddrLen = sizeof(struct sockaddr_in6);

    // IPv4 also work.
    cliFd = ::accept(getFd(), cliAddr, &cliAddrLen);
    if (cliFd == -1)
    {
        if (errno == EWOULDBLOCK)
        {
            return eCodes::ST_RetryLater;
        }

        throw std::system_error(errno, std::system_category(),
                                "Listener::doAccept");
    }

    if (AF_INET == cliAddr->sa_family)
    {
        struct sockaddr_in* cliAddr4 =
            reinterpret_cast<struct sockaddr_in*>(&cliAddr6);
        cliIP.setIP(cliAddr4->sin_addr);
        cliPort = cliAddr4->sin_port;
    }
    else
    {
        cliIP.setIP(cliAddr6.sin6_addr);
        cliPort = cliAddr6.sin6_port;
    }

    return eCodes::ST_Ok;
}
}
