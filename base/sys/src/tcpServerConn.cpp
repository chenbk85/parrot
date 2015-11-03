#include "ioEvent.h"
#include "tcpServerConn.h"

namespace parrot
{
TcpServerConn::TcpServerConn() : _remoteIP(), _remotePort(0)
{
}

void TcpServerConn::setRemoteAddr(const std::string& ip)
{
    _remoteIP = ip;
}

void TcpServerConn::setRemoteAddr(std::string&& ip)
{
    _remoteIP = std::move(ip);
}

void TcpServerConn::setRemotePort(uint16_t port)
{
    _remotePort = port;
}

const std::string & TcpServerConn::getRemoteAddr() const
{
    return _remoteIP;
}

uint16_t TcpServerConn::getRemotePort() const
{
    return _remotePort;
}
}
