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
        void reset();
        void setConnected();
        bool isConnected();
        void send(const char* buf, uint32_t len, uint32_t &sentLen);
        void recv(char* buf, uint32_t len, uint32_t &recvLen);

      protected:
        std::string  _srvIp;
        uint16_t     _srvPort;
    };
}

#endif
