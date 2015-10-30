#include <stdexcept>
#include <system_error>
#include <cstring>

#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

#include "ipHelper.h"

namespace parrot
{
IPHelper::IPHelper()
    : _version(-1), _ip(), _addr6(in6addr_any), _addr4(INADDR_ANY)
{
}

IPHelper::IPHelper(const std::string& ip)
    : _version(-1), _ip(ip), _addr6(), _addr4()
{
    std::memset(&_addr6, 0, sizeof(struct in6_addr));
    std::memset(&_addr4, 0, sizeof(struct in_addr));
    setIP(ip);
}

void IPHelper::setIP(const std::string& ip)
{
    if (ip.length() == 0)
    {
        _addr6 = in6addr_any;
        _addr4 = INADDR_ANY;
        _version = 6;
        return;
    }
    
    if (ip.find(".") != std::string::npos)
    {
        int ret = inet_pton(AF_INET, ip.c_str(), &_addr4);
        if (ret != 1)
        {
            throw std::runtime_error(std::string("Bad IP") + ip);
        }

        _version = 4;
    }
    else if (ip.find(":") != std::string::npos)
    {
        int ret = inet_pton(AF_INET6, ip.c_str(), &_addr6);
        if (ret != 1)
        {
            throw std::runtime_error(std::string("Bad IP") + ip);
        }
        _version = 6;
    }
    else
    {
        throw std::runtime_error(std::string("Bad IP") + ip);
    }

    _ip = ip;
}

void IPHelper::setIP(const struct in6_addr& addr)
{
    char buff[INET6_ADDRSTRLEN];
    if (!inet_ntop(AF_INET6, &addr, buff, sizeof(struct in6_addr)))
    {
        throw std::system_error(errno, std::system_category(),
                                "IPHelper::setIP ");
    }

    _ip = buff;
    _version = 6;
}

void IPHelper::setIP(const struct in_addr& addr)
{
    char buff[INET_ADDRSTRLEN];
    if (!inet_ntop(AF_INET, &addr, buff, sizeof(struct in_addr)))
    {
        throw std::system_error(errno, std::system_category(),
                                "IPHelper::setIP ");
    }
    _ip = buff;
    _version = 4;
}

bool IPHelper::isIPv4() const noexcept
{
    return _version == 4;
}

bool IPHelper::isIPv6() const noexcept
{
    return _version == 6;
}

struct in_addr IPHelper::getIPv4Bin() const noexcept
{
    return _addr4;
}

struct in6_addr IPHelper::getIPv6Bin() const noexcept
{
    return _addr6;
}

std::string IPHelper::getIPStr() const noexcept
{
    return _ip;
}

void IPHelper::getIPAddress(const std::string& domainName,
                            std::list<std::string>& ipv4List,
                            std::list<std::string>& ipv6List)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    char buff[INET6_ADDRSTRLEN];

    std::memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Tcp socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0; /* Any protocol */
    hints.ai_canonname = nullptr;
    hints.ai_addr = nullptr;
    hints.ai_next = nullptr;

    int s = getaddrinfo(domainName.c_str(), nullptr, &hints, &result);
    if (s != 0)
    {
        if (result != nullptr)
        {
            freeaddrinfo(result);
        }

        throw std::system_error(s, std::system_category(),
                                std::string("getIPAddress: ") +
                                    gai_strerror(s));
    }

    for (rp = result; rp != nullptr; rp = rp->ai_next)
    {
        if (AF_INET == rp->ai_family)
        {
            struct sockaddr_in* addr_in = (struct sockaddr_in*)(rp->ai_addr);
            (void)inet_ntop(rp->ai_family, &addr_in->sin_addr.s_addr, buff,
                            sizeof(buff));
            ipv4List.emplace_back(buff);
        }
        else if (AF_INET6 == rp->ai_family)
        {
            struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)(rp->ai_addr);
            (void)inet_ntop(rp->ai_family, addr_in6->sin6_addr.s6_addr, buff,
                            sizeof(buff));
            ipv6List.emplace_back(buff);
        }
    }

    freeaddrinfo(result);
}
}
