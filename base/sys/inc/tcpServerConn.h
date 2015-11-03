#ifndef __BASE_SYS_INC_TCPSERVERCONN_H__
#define __BASE_SYS_INC_TCPSERVERCONN_H__

#include <string>
#include <cstdint>

#include "ioEvent.h"

namespace parrot
{
class TcpServerConn : public IoEvent
{
  public:
    TcpServerConn();
    TcpServerConn(const TcpServerConn&) = delete;
    TcpServerConn& operator=(const TcpServerConn&) = delete;

  public:
    void setRemoteAddr(const std::string& ip);
    void setRemoteAddr(std::string&& ip);
    const std::string &getRemoteAddr() const;

    void setRemotePort(uint16_t port);
    uint16_t getRemotePort() const;

  protected:
    std::string _remoteIP;
    uint16_t _remotePort;
};
}

#endif
