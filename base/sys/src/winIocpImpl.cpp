#if defined(_WIN32)

namespace parrot
{
    WinIocpImpl::WinIocpImpl(uint32_t threadNum, uint32_t dequeueCount):
        _comletionPort(INVALID_HANDLE_VALUE),
        _overlappedEntryArr(new OVERLAPPED_ENTRY[dequeueCount]),
        _threadNum(threadNum)
    {        
    }

    WinIocpImpl::~WinIocpImpl()
    {
    }
    
    void WinIocpImpl::create()
    {
        _comletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
                                                NULL, 0, _threadNum);
        if (_comletionPort == INVALID_HANDLE_VALUE)
        {
            throw std::system_error(
                GetLastError(), std::system_category(), 
                "WinIocpImpl::create::CreateIoCompletionPort");
        }
    }
}

#endif
