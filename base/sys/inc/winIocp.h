#ifndef __BASE_SYS_INC_WINIOCP_H__
#define __BASE_SYS_INC_WINIOCP_H__

#if defined(_WIN32)

namespace parrot
{
    class WinIocp
    {
      public:
        WinIocp(uint32_t threadNum, uint32_t dequeueCount);
        ~WinIocp();
        WinIocp(const WinIocp&) = delete;
        WinIocp& operator=(const WinIocp&) = delete;

      public:
        void create();
        

      private:
    };
}

#endif // _WIN32
#endif // __BASE_SYS_INC_WINIOCP_H__
