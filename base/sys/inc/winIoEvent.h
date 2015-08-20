#ifndef __BASE_SYS_INC_WINIOEVENT_H__
#define __BASE_SYS_INC_WINIOEVENT_H__

#if defined(_WIN32)

#include <WinSock2.h>

namespace parrot
{
    class WinIoEvent : public IoEvent
    {
      public:
        WinIoEvent();
        virtual ~WinIoEvent();
        WinIoEvent(const WinIoEvent&) = delete;
        WinIoEvent &operator=(const WinIoEvent&) = delete;

      public:
        SOCKET getSocket();
        void setSocket(SOCKET s);
        WSAOVERLAPPED* getOverLapped();
        WSABUF* getWSABuf();
        void close() override;
        void setBytesTransferred(uint32_t count);
      private:
        void postRead(WinIoEvent *ev);
        void postWrite(WinIoEvent *ev);

      private:
        WSAOVERLAPPED               _overLapped;
        WSABUF                      _wsaBuf;
        SOCKET                      _socket;
        Transport *                 _transport;
    };
}

#endif // _WIN32
#endif
