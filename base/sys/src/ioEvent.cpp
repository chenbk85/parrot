#if defined(__linux__)
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#elif defined(__APPLE__)
#include <unistd.h>
#include <fcntl.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#elif defined(_WIN32)

#endif

#include <system_error>
#include <string>

#include "macroFuncs.h"
#include "ioEvent.h"

namespace parrot
{
IoEvent::IoEvent()
    : _fd(-1),
      _uniqueKey(0),
      _nextAction(eIoAction::None),
      _currAction(eIoAction::None),
      _notifiedAction(eIoAction::None),
      _isError(false),
      _isEof(false),
      _remoteIP(),
      _remotePort()
{
}

IoEvent::~IoEvent()
{
    close();
}

void IoEvent::setFd(int fd)
{
    _fd = fd;
}

int IoEvent::getFd() const
{
    return _fd;
}

bool IoEvent::isConnection() const
{
    return true;
}

eIoAction IoEvent::getDefaultAction() const
{
    return eIoAction::Read;
}

void IoEvent::setNextAction(eIoAction act)
{
    PARROT_ASSERT(act != eIoAction::None);
    _nextAction = act;
}

eIoAction IoEvent::getNextAction() const
{
    return _nextAction;
}

void IoEvent::setCurrAction(eIoAction act)
{
    PARROT_ASSERT(act != eIoAction::None);    
    _currAction = act;
}

eIoAction IoEvent::getCurrAction() const
{
    return _currAction;
}

void IoEvent::setNotifiedAction(eIoAction act)
{
    PARROT_ASSERT(act != eIoAction::None);    
    _notifiedAction = act;
}

eIoAction IoEvent::getNotifiedAction() const
{
    return _notifiedAction;
}

bool IoEvent::sameAction() const
{
    return _currAction == _nextAction;
}

void IoEvent::setError(bool isErr)
{
    _isError = isErr;
}

bool IoEvent::isError() const
{
    return _isError;
}

void IoEvent::setEof(bool isEof)
{
    _isEof = isEof;
}

bool IoEvent::isEof() const
{
    return _isEof;
}

void IoEvent::setUniqueKey(uint64_t key)
{
    _uniqueKey = key;
}

uint64_t IoEvent::getUniqueKey() const
{
    return _uniqueKey;
}

bool IoEvent::isReadAvail() const
{
    if (_notifiedAction == eIoAction::Read ||
        _notifiedAction == eIoAction::ReadWrite)
    {
        return true;
    }

    return false;
}

bool IoEvent::isWriteAvail() const
{
    if (_notifiedAction == eIoAction::Write ||
        _notifiedAction == eIoAction::ReadWrite)
    {
        return true;
    }

    return false;
}

void IoEvent::close()
{
    if (_fd >= 0)
    {
        ::shutdown(_fd, SHUT_RDWR);
        ::close(_fd);
        _fd = -1;
    }
}

void IoEvent::setRemoteAddr(const std::string& ip)
{
    _remoteIP = ip;
}

void IoEvent::setRemoteAddr(std::string&& ip)
{
    _remoteIP = std::move(ip);
}

void IoEvent::setRemotePort(uint16_t port)
{
    _remotePort = port;
}

const std::string & IoEvent::getRemoteAddr() const
{
    return _remoteIP;
}

uint16_t IoEvent::getRemotePort() const
{
    return _remotePort;
}

#if defined(__APPLE__) || defined(__linux__)
/////////////////////////////////////////////////////////////////////////
/// Send/Recv/Read/Write
//////////////
eCodes IoEvent::send(const unsigned char* buff, uint32_t buffLen, uint32_t& sentLen)
{
    if (!buff || buffLen == 0 || _fd < 0)
    {
        PARROT_ASSERT(0);
    }

    ssize_t ret = 0;
    while (true)
    {
        ret = ::send(_fd, buff, buffLen, 0);
        if (ret != -1)
        {
            sentLen = ret;
            break;
        }

        if (errno == EAGAIN)
        {
            sentLen = 0;
            break;
        }

        if (errno == EINTR)
        {
            continue;
        }

        throw std::system_error(errno, std::system_category(), "IoEvent::send");
    }

    return eCodes::ST_Ok;
}

eCodes IoEvent::recv(unsigned char* buff, uint32_t buffLen, uint32_t& rcvdLen)
{
    if (!buff || buffLen == 0 || _fd < 0)
    {
        PARROT_ASSERT(0);
    }

    ssize_t ret = 0;
    while (true)
    {
        ret = ::recv(_fd, buff, buffLen, 0);
        if (ret != -1)
        {
            rcvdLen = ret;
            break;
        }

        if (errno == EAGAIN)
        {
            rcvdLen = 0;
            break;
        }

        if (errno == EINTR)
        {
            continue;
        }

        throw std::system_error(errno, std::system_category(), "IoEvent::recv");
    }

    return eCodes::ST_Ok;
}
#endif
}
