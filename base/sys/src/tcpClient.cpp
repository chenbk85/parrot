#include "tcpClient.h"

namespace parrot
{
    TcpClient::TcpClient():
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

    }
}
