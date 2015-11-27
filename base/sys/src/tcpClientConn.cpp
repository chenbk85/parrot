#include <system_error>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>

#include "sysHelper.h"
#include "ipHelper.h"
#include "ioEvent.h"
#include "tcpClientConn.h"

namespace parrot
{
TcpClientConn::TcpClientConn() : IoEvent(), _connected(false)
{
}

TcpClientConn::~TcpClientConn()
{
    disconnect();
}

void TcpClientConn::connect()
{
    IPHelper ipHelper;
    ipHelper.setIP(getRemoteAddr());

    int fd = -1;

    if (ipHelper.isIPv4())
    {
        fd = ::socket(AF_INET, SOCK_STREAM, 0);
    }
    else
    {
        fd = ::socket(AF_INET6, SOCK_STREAM, 0);
    }

    if (fd < 0)
    {
        throw std::system_error(errno, std::system_category(),
                                "TcpClientConn::connect: socket");
    }

    setNonBlock(fd);

    socklen_t addrLen = 0;
    struct sockaddr* addr = nullptr;
    struct sockaddr_in addr4;
    struct sockaddr_in6 addr6;

    if (ipHelper.isIPv4())
    {
        addrLen = sizeof(struct sockaddr_in);
        addr = (struct sockaddr*)&addr4;
        addr4.sin_family = AF_INET;
        addr4.sin_port = htons(getRemotePort());
        addr4.sin_addr = ipHelper.getIPv4Bin();
        std::memset(addr4.sin_zero, 0, sizeof(addr4.sin_zero));
    }
    else
    {
        addrLen = sizeof(struct sockaddr_in6);
        addr = (struct sockaddr*)&addr6;
        addr6.sin6_family = AF_INET6;
        addr6.sin6_port = htons(getRemotePort());
        addr6.sin6_addr = ipHelper.getIPv6Bin();
    }

    if (::connect(fd, addr, addrLen) == 0 || EISCONN == errno)
    {
        setFd(fd);
        _connected = true;
    }
}

void TcpClientConn::setConnected() noexcept
{
    _connected = true;
}

bool TcpClientConn::isConnected() const noexcept
{
    return _connected;
}

void TcpClientConn::disconnect()
{
    close();
    _connected = false;
}
}
