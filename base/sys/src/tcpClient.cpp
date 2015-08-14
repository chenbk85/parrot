#include <system_error>

#include <sys/types.h>
#include <sys/socket.h>

#include "ioEvent.h"
#include "tcpClient.h"

namespace parrot
{
    TcpClient::TcpClient():
        IoEvent(),
        _srvIp(),
        _srvPort(0)
    {
    }

    TcpClient::~TcpClient()
    {
    }

    void TcpClient::connect(const std::string &srvIp, uint16_t srvPort)
    {
        _srvIp = srvIp;
        _srvPort = srvPort;

        int fd = ::socket(AF_INET, SOCK_STREAM, 0);

        if (fd < 0)
        {
            throw std::system_error(errno, std::system_category(),
                                    "TcpClient::connect: socket");
        }

        setNonBlock(fd);

        socklen_t addrLen = sizeof(struct sockaddr_in);
        struct sockaddr* addr; 
        struct sockaddr_in addrIn; 

        addr = (struct sockaddr*)&peer_addr_in;
        addrIn.sin_family = AF_INET;
        addrIn.sin_port = htons(_srvPort);
        addrIn.sin_addr.s_addr = ;
        memset(addrIn.sin_zero, 0, sizeof(addrIn.sin_zero));

        struct sockaddr addr;
        addr

        int ret = connect(fd, );

    }

    void TcpClient::setConnected()
    {

    }

    bool TcpClient::isConnected()
    {

    }
}
