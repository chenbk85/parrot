#include <unistd.h>
#include <system_error>
#include "sysHelper.h"
#include "ioEvent.h"
#include "eventTrigger.h"

namespace parrot
{
EventTrigger::EventTrigger()
    : IoEvent{}, _pipeFds{-1, -1}, _buffer(new unsigned char[4096])
{
}

EventTrigger::~EventTrigger()
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

void EventTrigger::create()
{
    int ret = ::pipe(_pipeFds);
    if (ret < 0)
    {
        throw std::system_error(errno, std::system_category(),
                                "EventTrigger::create");
    }

    // Make fd for reading nonblocking.
    setNonBlock(_pipeFds[0]);
    setFd(_pipeFds[0]);
    setNextAction(eIoAction::Read);
}

void EventTrigger::trigger()
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
                                "EventTrigger::trigger");
    }
}

void EventTrigger::acknowledge()
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
                                "EventTrigger::acknowledge");
    }
}

bool EventTrigger::isConnection() const
{
    return false;
}

eIoAction EventTrigger::handleIoEvent()
{
    if (isError())
    {
        throw std::system_error(errno, std::system_category(),
                                "EventTrigger::handleIoEvent: error.");
    }

    if (isEof())
    {
        throw std::system_error(errno, std::system_category(),
                                "EventTrigger::handleIoEvent: eof.");
    }

    acknowledge();
    return eIoAction::Read;
}
}
