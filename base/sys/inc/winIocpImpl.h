#ifndef __BASE_SYS_INC_WINIOCPIMPL_H__
#define __BASE_SYS_INC_WINIOCPIMPL_H__

#if defined(_WIN32)

#include <memory>
#include <cstdint>

namespace parrot
{
    class WinIocpImpl
    {
      public:
        WinIocpImpl(uint32_t threadNum, uint32_t dequeueCount);
        ~WinIocpImpl();
        WinIocpImpl(const WinIocpImpl&) = delete;
        WinIocpImpl& operator=(const WinIocpImpl&) = delete;

      public:
        static HANDLE createIocp(uint32_t threadNum);

      public:
        void waitIoEvents(int32_t ms);
        void addEvent(IoEvent *ev);
        void monitorRead(IoEvent *ev);
        void monitorWrite(IoEvent *ev);
        IoEvent* getIoEvent(uint32_t idx) const noexcept;
        void stopWaiting();
        void close();
        
      private:
        HANDLE                                _completionPort;
        std::unique<OVERLAPPED_ENTRY []>      _overlappedEntryArr;
        uint32_t                              _threadNum;
    };
}

#endif // _WIN32
#endif // __BASE_SYS_INC_WINIOCPIMPL_H__
