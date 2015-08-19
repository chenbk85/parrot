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
        void create();
        
      private:
        HANDLE                                _completionPort;
        std::unique<OVERLAPPED_ENTRY []>      _overlappedEntryArr;
        uint32_t                              _threadNum;
    };
}

#endif // _WIN32
#endif // __BASE_SYS_INC_WINIOCPIMPL_H__
