#ifndef __BASE_SYS_INC_TCPCLIENTCONN_H__
#define __BASE_SYS_INC_TCPCLIENTCONN_H__

#include <cstdint>
#include <string>

namespace parrot
{
class IoEvent;

class TcpClientConn : public IoEvent
{
  public:
    TcpClientConn();
    virtual ~TcpClientConn();
    TcpClientConn(const TcpClientConn&) = delete;
    TcpClientConn& operator=(const TcpClientConn&) = delete;

  public:
    void connect();
    void setConnected() noexcept;
    bool isConnected() const noexcept;
    void disconnect();

  protected:
    bool _connected;
};
}

#endif
