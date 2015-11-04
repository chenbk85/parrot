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
};
}

#endif
