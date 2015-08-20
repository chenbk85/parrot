#ifndef __BASE_SYS_INC_WINIOCP_H__
#define __BASE_SYS_INC_WINIOCP_H__

#if defined(_WIN32)

namespace parrot
{
    class WinIocpImpl;

    class WinIocp : public EventNotifier
    {
      public:
        WinIocp(HANDLE iocp, uint32_t dequeueCount);
        ~WinIocp();
        WinIocp(const WinIocp&) = delete;
        WinIocp& operator=(const WinIocp&) = delete;

      public:
        void create() override;

        uint32_t waitIoEvents(int32_t ms) override;

        void addEvent(IoEvent *ev) override;
        
        void monitorRead(IoEvent *ev) override;

        void monitorWrite(IoEvent *ev) override;

        void delEvent(IoEvent *ev) override;

        IoEvent* getIoEvent(uint32_t idx) const override;

        void stopWaiting() override;

      private:
        WinIocpImpl *_impl;
    };
}

#endif // _WIN32
#endif // __BASE_SYS_INC_WINIOCP_H__
