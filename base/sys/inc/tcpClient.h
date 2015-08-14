#ifndef __BASE_SYS_INC_TCPCLIENT_H__
#define __BASE_SYS_INC_TCPCLIENT_H__

#include <cstdint>
#include <string>

namespace parrot
{
    class IoEvent;

    class TcpClient : public IoEvent
    {
      public:
        TcpClient();
        virtual ~TcpClient();
        TcpClient(const TcpClient&) = delete;
        TcpClient& operator=(const TcpClient&) = delete;

      public:
        void connect(const std::string &srvIp, uint16_t srvPort);
        void setConnected() noexcept;
        bool isConnected() const noexcept;
        void disconnect();

      protected:
        std::string  _srvIp;
        uint16_t     _srvPort;
        bool         _connected;
    };
}

#endif
