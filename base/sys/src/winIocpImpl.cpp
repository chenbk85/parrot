#if defined(_WIN32)
#include <cassert.h>
#include "macroFuncs.h"

#include "winIocpImpl.h"

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

    uint32_t WinIocpImpl::waitIoEvents(int32_t ms)
    {
        void *lpCompletionKey = NULL;
        ULONG numberRemoved = 0;
        bool ret = false;

        if (ms <= 0)
        {
            ret = (bool)GetQueuedCompletionStatusEx(
                _comletionPort, _overlappedEntryArr.get(), _dequeueCount,
                &numberRemoved, INFINITE, false);
        }
        else
        {
            ret = (bool)GetQueuedCompletionStatusEx(
                _comletionPort, _overlappedEntryArr.get(), _dequeueCount,
                &numberRemoved, ms, false);
        }

        if (!ret)
        {
            throw std::system_error(
                GetLastError(), std::system_category(), 
                "WinIocpImpl::waitIoEvents");            
        }

        return static_cast<uint32_t>(numberRemoved);
    }

    void WinIocpImpl::addEvent(WinIoEvent *ev)
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
            eIoAction act = ev->getAction();
            if (act == eIoAction::Read)
            {
                postRead(ev)
            }
            else if (act == eIoAction::Write)
            {
                postWrite(ev);
            }
            else
            {
                assert(false);
            }
        }
    }

    void WinIocpImpl::postRead(WinIoEvent *ev)
    {
        DWORD dwFlags = 0;
        int ret = WSARecv(ev->getSocket(), ev->getWSABuf(), 1,
                          NULL, &dwFlags, ev->getOverLapped(), NULL);

        auto err = WSAGetLastError();
        if (ret == SOCKET_ERROR && (ERROR_IO_PENDING != err))
        {
            throw std::system_error(err, std::system_category(), 
                "WinIocpImpl::postRead");
        }
    }

    void WinIocpImpl::postWrite(WinIoEvent *ev)
    {
        DWORD dwFlags = 0;
        int ret = WSASend(ev->getSocket(), ev->getWSABuf(), 1,
                          NULL, &dwFlags, ev->getOverLapped(), NULL);

        auto err = WSAGetLastError();
        if (ret == SOCKET_ERROR && (ERROR_IO_PENDING != err))
        {
            throw std::system_error(err, std::system_category(),
                "WinIocpImpl::postWrite");
        }
    }

    WinIoEvent* WinIocpImpl::getIoEvent(uint32_t idx) const
    {
        WinIoEvent *ev = (WinIoEvent*)_overlappedEntryArr[idx].Internal;
        ev->setBytesTransferred(
            _overlappedEntryArr[idx].dwNumberOfBytesTransferred);
        return ev;
    }

    void WinIocpImpl::stopWaiting()
    {
        PostQueuedCompletionStatus(_comletionPort, 0, NULL, NULL);
    }
}

#endif
