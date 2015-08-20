#ifndef __BASE_SYS_INC_WINIOCPIMPL_H__
#define __BASE_SYS_INC_WINIOCPIMPL_H__

#if defined(_WIN32)

#include <memory>
#include <cstdint>

namespace parrot
{
    class WinIoEvent;

    class WinIocpImpl
    {
      public:
        WinIocpImpl(HANDLE iocp, uint32_t dequeueCount);
        ~WinIocpImpl();
        WinIocpImpl(const WinIocpImpl&) = delete;
        WinIocpImpl& operator=(const WinIocpImpl&) = delete;

      public:
        static fileHdl createIocp(uint32_t threadNum);

      public:
        uint32_t waitIoEvents(int32_t ms);
        void addEvent(WinIoEvent *ev);
        WinIoEvent* getIoEvent(uint32_t idx) const noexcept;
        void stopWaiting();
        
      private:
        HANDLE                                _completionPort;
        std::unique<OVERLAPPED_ENTRY []>      _overlappedEntryArr;
        uint32_t                              _dequeueCount;
    };
}

#endif // _WIN32
#endif // __BASE_SYS_INC_WINIOCPIMPL_H__
