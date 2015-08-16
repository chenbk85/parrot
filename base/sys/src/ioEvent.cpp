#include <unistd.h>
#include <fcntl.h>

#if defined(__linux__)
#include <sys/epoll.h>
#elif defined(__APPLE__)
#include <sys/event.h>
#include <sys/time.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>

#include <system_error>
#include <string>

#include "macroFuncs.h"
#include "ioEvent.h"



namespace parrot
{
    IoEvent::IoEvent() noexcept:
    _fd(-1),
        _filter(-1),
        _flags(-1),
        _action(eIoAction::None)
    {
    }

    IoEvent::~IoEvent()
    {
        close();
    }

    void IoEvent::setFd(int fd) noexcept
    {
        _fd = fd;
    }

    int IoEvent::getFd() const noexcept
    {
        return _fd;
    }

    void IoEvent::setAction(eIoAction act) noexcept
    {
        _action = act;
    }

    void IoEvent::setIoRead() noexcept
    {
#if defined(__linux__)
        _filter = EPOLLIN|EPOLLRDHUP|EPOLLET;
#elif defined(__APPLE__)
        _filter = EVFILT_READ;
#endif
        _action = eIoAction::Read;
    }

    void IoEvent::setIoWrite() noexcept
    {
#if defined(__linux__)
        _filter = EPOLLOUT|EPOLLRDHUP|EPOLLET;
#elif defined(__APPLE__)
        _filter = EVFILT_WRITE;
#endif
        _action = eIoAction::Write;
    }

    eIoAction IoEvent::getAction() const noexcept
    {
        return _action;
    }

    int IoEvent::getFilter() const noexcept
    {
        return _filter;
    }

    void IoEvent::setFilter(int filter) noexcept
    {
        _filter = filter;
    }

    void IoEvent::setFlags(int flags) noexcept
    {
        _flags = flags;
    }

    int IoEvent::getFlags() const noexcept
    {
        return _flags;
    }

    bool IoEvent::isError() const noexcept
    {
#if defined(__linux__)
        if (_filter & EPOLLERR || _filter & EPOLLHUP)
        {
            return true;
        }

        return false;
#elif defined(__APPLE__)
        if (_flags & EV_ERROR)
        {
            return true;
        }

        return false;
#endif
    }

    bool IoEvent::isEof() const noexcept
    {
#if defined(__linux__)
        if (_filter & EPOLLRDHUP || _filter & EPOLLHUP)
        {
            return true;
        }

        return false;
#elif defined(__APPLE__)
        if (_flags & EV_EOF)
        {
            return true;
        }

        return false;
#endif        
    }

    bool IoEvent::isReadAvail() const noexcept
    {
#if defined(__linux__)
        if (_filter & EPOLLIN)
        {
            return true;
        }

        return false;
#elif defined(__APPLE__)
        if (_filter == EVFILT_READ)
        {
            return true;
        }

        return false;
#endif
    }

    bool IoEvent::isWriteAvail() const noexcept
    {
#if defined(__linux__)
        if (_filter & EPOLLOUT)
        {
            return true;
        }

        return false;
#elif defined(__APPLE__)
        if (_filter == EVFILT_WRITE)
        {
            return true;
        }

        return false;
#endif
    }

    void IoEvent::close() noexcept
    {
        if (_fd >= 0) 
        {
            ::close(_fd);
            _fd = -1;
            _filter = -1;
            _flags = -1;
            _action = eIoAction::None;
        }
    }

    /////////////////////////////////////////////////////////////////////////
    /// Send/Recv/Read/Write
    //////////////
    uint32_t IoEvent::send(const char* buff, uint32_t buffLen)
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
                break;
            }

            if (errno == EAGAIN)
            {
                continue;
            }

            throw std::system_error(errno, std::system_category(),
                                    "IoEvent::send");
        }

        return ret;
    }

    uint32_t IoEvent::recv(char* buff, uint32_t buffLen)
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
                break;
            }

            if (errno == EAGAIN)
            {
                continue;
            }

            throw std::system_error(errno, std::system_category(),
                                    "IoEvent::recv");
        }

        return ret;
    }

    /////////////////////////////////////////////////////////////////////////
    /// Static functions.
    //////////////
    void IoEvent::manipulateFd(int fd, int flags)
    {
        int oldFlags = ::fcntl(fd, F_GETFL, 0);
        if (oldFlags < 0)
        {
            throw std::system_error(errno, std::system_category(),
                                    "IoEvent::manipulateFd: Get " +
                                    std::to_string(flags));
        }

        int newFlags = oldFlags | flags;
        if (::fcntl(fd, F_SETFL, newFlags) < 0)
        {
            throw std::system_error(errno, std::system_category(),
                                    "IoEvent::manipulateFd: Set " +
                                    std::to_string(flags));
        }
    }

    void IoEvent::setNonBlock(int fd)
    {
        manipulateFd(fd, O_NONBLOCK);
    }

    void IoEvent::setNoDelay(int fd)
    {
        manipulateFd(fd, O_NDELAY);
    }
}
