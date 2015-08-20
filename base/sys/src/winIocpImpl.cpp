#if defined(_WIN32)

#include "macroFuncs.h"

namespace parrot
{
    WinIocpImpl::WinIocpImpl(HANDLE iocp, uint32_t dequeueCount):
        _comletionPort(iocp),
        _overlappedEntryArr(new OVERLAPPED_ENTRY[dequeueCount]),
        _dequeueCount(dequeueCount)
    {
        PARROT_ASSERT(_comletionPort != INVALID_HANDLE_VALUE && 
                      dequeueCount > 0);
    }

    WinIocpImpl::~WinIocpImpl()
    {
    }
    
    HANDLE WinIocpImpl::createIocp(uint32_t threadNum)
    {
        HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
                                             NULL, 0, threadNum);
        if (iocp == INVALID_HANDLE_VALUE)
        {
            throw std::system_error(GetLastError(), std::system_category(), 
                                    "WinIocpImpl::createIocp");
        }

        return iocp;
    }

    void WinIocpImpl::waitIoEvents(int32_t ms)
    {
        void *lpCompletionKey = NULL;
        ULONG numberRemoved = 0;
        BOOL ret = FALSE;

        if (ms <= 0)
        {
            ret = GetQueuedCompletionStatusEx(
                _comletionPort, _overlappedEntryArr.get(), _dequeueCount,
                &numberRemoved, INFINITE, false);
        }
        else
        {
            ret = GetQueuedCompletionStatusEx(
                _comletionPort, _overlappedEntryArr.get(), _dequeueCount,
                &numberRemoved, ms, false);
        }

        if (!ret)
        {
            throw std::system_error(
                GetLastError(), std::system_category(), 
                "WinIocpImpl::waitIoEvents");            
        }
    }

    void WinIocpImpl::addEvent(IoEvent *ev)
    {
        if (CreateIoCompletionPort(ev->getHandle(), _comletionPort, 
                                   (ULONG_PTR)ev, 0) == NULL)
        {
            throw std::system_error(
                GetLastError(), std::system_category(), 
                "WinIocpImpl::waitIoEvents");
        }
        else
        {
            cout << "CreateIoCompletionPort Client success." << endl;

            
            ZeroMemory(&data->Overlapped, sizeof(data->Overlapped));
            ZeroMemory(data->Buffer, sizeof(data->Buffer));

            data->opCode = IO_READ;
            data->nTotalBytes = 0;
            data->wsabuf.buf = data->Buffer;
            data->wsabuf.len = sizeof(data->Buffer);
            data->activeSocket = ls;
            DWORD dwFlags = 0;
            int nRet = WSARecv(ls, &data->wsabuf, 1,
                               NULL, &dwFlags, &data->Overlapped, NULL);

            if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
            {
                cout << "WASRecvFailed:: Reason Code::"
                     << WSAGetLastError() << endl;
                closesocket(ls);
                delete data;
            }
            else
            {
                cout << "WSARecv success." << endl;
            }
        }
    }

    void WinIocpImpl::monitorRead(IoEvent *ev)
    {

    }

    void WinIocpImpl::monitorWrite(IoEvent *ev)
    {

    }

    IoEvent* WinIocpImpl::getIoEvent(uint32_t idx) const noexcept
    {

    }

    void WinIocpImpl::stopWaiting()
    {

    }

    void WinIocpImpl::close()
    {

    }
}

#endif
