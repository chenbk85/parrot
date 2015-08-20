#if defined(_WIN32)

#include <cstring>

#include "macroFuncs.h"
#include "winIoEvent.h"

namespace parrot
{
    WinIoEvent::WinIoEvent():
        IoEvent(),
        _overLapped(),
        _wsaBuf(),
        _socket(INVALID_SOCKET),
        _transport(nullptr)
    {
        std::memset(&_overLapped, sizeof(_overLapped), 0);
        std::memset(&_wsaBuf, sizeof(_wsaBuf), 0);
    }

    WinIoEvent::~WinIoEvent()
    {
    }

    SOCKET WinIoEvent::getSocket()
    {
        return _socket;
    }

    void WinIoEvent::setSocket(SOCKET s)
    {
        _socket = s;
    }

    WSAOVERLAPPED* WinIoEvent::getOverLapped()
    {
        return &_overLapped;
    }

    WSABUF* WinIoEvent::getWSABuf()
    {
        if (_action == eIoAction::Read)
        {
            _transport->getRecvBuff(_wsaBuf.buf, _wsaBuf.len);
        }
        else if (_action == eIoAction::Write)
        {
            _transport->getSendBuff(_wsaBuf.buf, _wsaBuf.len);
        }
        else
        {
            PARROT_ASSERT(false);
        }

        return &_wsaBuf;
    }

    void WinIoEvent::setBytesTransferred(uint32_t count)
    {
        if (_action == eIoAction::Read)
        {
            _transport->setRecvedLen(count);
        }
        else if (_action == eIoAction::Write)
        {
            _transport->setSentLen(count);
        }
        else
        {
            PARROT_ASSERT(false);
        }
    }
}

#endif
