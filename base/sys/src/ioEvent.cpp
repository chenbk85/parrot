#include <unistd.h>
#include <fcntl.h>

#include "ioEvent.h"

namespace parrot
{
    IoEvents::IoEvents() noexcept:
        _fd(-1),
        _epollEvents(-1)
    {
    }

    IoEvents::~IoEvents()
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

    void IoEvent::manipulateFd(int fd, int flags)
    {
        int oldFlags = ::fcntl(_fd, F_GETFL, 0);
        if (oldFlags < 0)
        {
            throw std::system_error(errno, std::system_category(),
                                    "IoEvent::manipulateFd: Get " +
                                    std::to_string(flags));
        }

        int newFlags = oldFlags | flags;
        if (::fcntl(_fd, F_SETFL, newFlags) < 0)
        {
            throw std::system_error(errno, std::system_category(),
                                    "IoEvent::manipulateFd: Set " +
                                    std::to_string(flags));
        }
    }

    void IoEvent::setNonBlock()
    {
        manipulateFd(_fd, O_NONBLOCK);
    }

    void IoEvent::setNoDelay()
    {
        manipulateFd(_fd, O_NDELAY);
    }

    int IoEvent::getEpollEvents() const noexcept
    {
        return _epollEvents;
    }

    void IoEvent::setEpollEvents(int events) const noexcept
    {
        _epollEvents = events;
    }
}
