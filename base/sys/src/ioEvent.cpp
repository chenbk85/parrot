#include <unistd.h>
#include <fcntl.h>
#include <system_error>

#include "ioEvent.h"

namespace parrot
{
    IoEvent::IoEvent() noexcept:
        _fd(-1),
        _epollEvents(-1)
    {
    }

    IoEvent::~IoEvent()
    {
    }

    void IoEvent::setFd(int fd) noexcept
    {
        _fd = fd;
    }

    int IoEvent::getFd() const noexcept
    {
        return _fd;
    }

    void IoEvent::setIoRead() noexcept
    {
#if defined(__linux__)
        _filter = EPOLLIN|EPOLLRDHUP|EPOLLET;
#elif defined(__APPLE__)
        _filter = EVFILT_READ;
#endif
    }

    void IoEvent::setIoWrite() noexcept
    {
#if defined(__linux__)
        _filter = EPOLLOUT|EPOLLRDHUP|EPOLLET;
#elif defined(__APPLE__)
        _filter = EVFILT_WRITE;
#endif
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
        if (_filter & EPOLLRD)
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
