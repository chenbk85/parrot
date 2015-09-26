#ifndef __BASE_SYS_INC_TCPSERVER_H__
#define __BASE_SYS_INC_TCPSERVER_H__

#include <string>
#include <cstdint>

#include "ioEvent.h"

namespace parrot {
class TcpServer : public IoEvent {
  public:
    TcpServer();
    TcpServer(const TcpServer &) = delete;
    TcpServer &operator=(const TcpServer &) = delete;

  public:
    void setRemoteAddr(const std::string &ip, uint16_t port);

  protected:
    std::string _remoteIP;
    uint16_t _remotePort;
};
}

#endif
