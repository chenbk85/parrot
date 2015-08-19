#include "ioEvent.h"
#include "tcpServer.h"

namespace parrot
{
    TcpServer::TcpServer():
        _remoteIP(),
        _remotePort(0)
    {
    }

    void TcpServer::setRemoteAddr(const std::string &ip, uint16_t port)
    {
        _remoteIP = ip;
        _remotePort = port;
    }
}
