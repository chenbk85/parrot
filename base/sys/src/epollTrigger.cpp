#include <unistd.h>
#include <system_error>

#include "epollTrigger.h"

namespace parrot
{
    EpollTrigger::EpollTrigger():
        IoEvent{},
        _pipeFds{-1, -1},
        _buffer(new unsigned char[4096])
    {
    }

    EpollTrigger::~EpollTrigger()
    {
        if (_pipeFds[0] != -1)
        {
            ::close(_pipeFds[0]);
            _pipeFds[0] = -1;
        }
        
        if (_pipeFds[1] != -1)
        {
            ::close(_pipeFds[1]);
            _pipeFds[1] = 1;
        }

        setFd(-1);
    }

    void EpollTrigger::create()
    {
        int ret = ::pipe(_pipeFds);
        if (ret < 0)
        {
            throw std::system_error(errno, std::system_category(),
                                    "EpollTrigger::create");
        }

        // Make fd for reading nonblocking.
        setNonBlock(_pipeFds[0]);
        setFd(_pipeFds[0]);
    }

    void EpollTrigger::trigger()
    {
        while (true)
        {
            // Write is blocking operation.
            int ret = ::write(_pipeFds[1], "a", 1);
            if (ret == 1)
            {
                break;
            }

            if (errno == EINTR)
            {
                continue;
            }

            throw std::system_error(errno, std::system_category(),
                                    "EpollTrigger::trigger");
        }
    }

    void EpollTrigger::acknowledge()
    {
        int ret = 0;
        
        while (true)
        {
            ret = ::read(_pipeFds[0], _buffer.get(), 4096);
            if (ret >= 0)
            {
                // Read all.
                continue;
            }

            if (errno == EINTR)
            {
                continue;
            }

            if (errno == EAGAIN)
            {
                break;
            }

            throw std::system_error(errno, std::system_category(),
                                    "EpollTrigger::acknowledge");
        }
    }

    eIoAction EpollTrigger::handleIoEvent()
    {
        acknowledge();
        return eIoAction::None;
    }
}
