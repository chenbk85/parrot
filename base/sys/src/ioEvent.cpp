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

namespace parrot {
IoEvent::IoEvent()
    : _fd(-1), _filter(-1), _flags(-1), _action(eIoAction::None), _remoteIp() {
}

IoEvent::~IoEvent() {
    close();
}

void IoEvent::setFd(int fd) {
    _fd = fd;
}

int IoEvent::getFd() const {
    return _fd;
}

void IoEvent::setAction(eIoAction act) {
    _action = act;
}

void IoEvent::setIoRead() {
#if defined(__linux__)
    _filter = EPOLLIN | EPOLLRDHUP | EPOLLET;
#elif defined(__APPLE__)
    _filter = EVFILT_READ;
#endif
    _action = eIoAction::Read;
}

void IoEvent::setIoWrite() {
#if defined(__linux__)
    _filter = EPOLLOUT | EPOLLRDHUP | EPOLLET;
#elif defined(__APPLE__)
    _filter = EVFILT_WRITE;
#endif
    _action = eIoAction::Write;
}

eIoAction IoEvent::getAction() const {
    return _action;
}

void IoEvent::setRemoteAddr(const std::string &ip) {
    _remoteIp = ip;
}

void IoEvent::setRemoteAddr(std::string &&ip) {
    _remoteIp = std::move(ip);
}

const std::string &IoEvent::getRemoteAddr() const {
    return _remoteIp;
}

int IoEvent::getFilter() const {
    return _filter;
}

void IoEvent::setFilter(int filter) {
    _filter = filter;
}

void IoEvent::setFlags(int flags) {
    _flags = flags;
}

int IoEvent::getFlags() const {
    return _flags;
}

bool IoEvent::isError() const {
#if defined(__linux__)
    if (_filter & EPOLLERR || _filter & EPOLLHUP) {
        return true;
    }

    return false;
#elif defined(__APPLE__)
    if (_flags & EV_ERROR) {
        return true;
    }

    return false;
#elif defined(_WIN32)
    return false;
#endif
}

bool IoEvent::isEof() const {
#if defined(__linux__)
    if (_filter & EPOLLRDHUP || _filter & EPOLLHUP) {
        return true;
    }

    return false;
#elif defined(__APPLE__)
    if (_flags & EV_EOF) {
        return true;
    }

    return false;
#else
    return false;
#endif
}

bool IoEvent::isReadAvail() const {
#if defined(__linux__)
    if (_filter & EPOLLIN) {
        return true;
    }

    return false;
#elif defined(__APPLE__)
    if (_filter == EVFILT_READ) {
        return true;
    }

    return false;
#elif defined(_WIN32)
    return false;
#endif
}

bool IoEvent::isWriteAvail() const {
#if defined(__linux__)
    if (_filter & EPOLLOUT) {
        return true;
    }

    return false;
#elif defined(__APPLE__)
    if (_filter == EVFILT_WRITE) {
        return true;
    }

    return false;
#elif defined(_WIN32)
    return false;
#endif
}

void IoEvent::close() {
    if (_fd >= 0) {
        ::shutdown(_fd, SHUT_RDWR);
        ::close(_fd);
        _fd = -1;
        _filter = -1;
        _flags = -1;
        _action = eIoAction::None;
    }
}

#if defined(__APPLE__) || defined(__linux__)
/////////////////////////////////////////////////////////////////////////
/// Send/Recv/Read/Write
//////////////
eCodes IoEvent::send(const char *buff, uint32_t buffLen, uint32_t &sentLen) {
    if (!buff || buffLen == 0 || _fd < 0) {
        PARROT_ASSERT(0);
    }

    ssize_t ret = 0;
    while (true) {
        ret = ::send(_fd, buff, buffLen, 0);
        if (ret != -1) {
            sentLen = ret;
            break;
        }

        if (errno == EAGAIN) {
            sentLen = 0;
            break;
        }

        if (errno == EINTR) {
            continue;
        }

        throw std::system_error(errno, std::system_category(), "IoEvent::send");
    }

    return eCodes::ST_Ok;
}

eCodes IoEvent::recv(char *buff, uint32_t buffLen, uint32_t &rcvdLen) {
    if (!buff || buffLen == 0 || _fd < 0) {
        PARROT_ASSERT(0);
    }

    ssize_t ret = 0;
    while (true) {
        ret = ::recv(_fd, buff, buffLen, 0);
        if (ret != -1) {
            rcvdLen = ret;
            break;
        }

        if (errno == EAGAIN) {
            rcvdLen = 0;
            break;
        }

        if (errno == EINTR) {
            continue;
        }

        throw std::system_error(errno, std::system_category(), "IoEvent::recv");
    }

    return eCodes::ST_Ok;
}
#endif

/////////////////////////////////////////////////////////////////////////
/// Static functions.
//////////////
#if defined(_WIN32)
void IoEvent::setExclusiveAddr(sockhdl fd) {
    BOOL optVal = TRUE;
    int optLen = sizeof(BOOL);

    int ret = setsockopt(fd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char *)&optVal,
                         optLen);
    if (ret != 0) {
        throw std::system_error(WSAGetLastError(), std::system_category(),
                                "WinIoEvent::setExclusiveAddr");
    }
}
#else
void IoEvent::setReuseAddr(sockhdl fd) {
    int optVal = 1;
    socklen_t optLen = sizeof(optVal);

    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&optVal, optLen);
    if (ret != 0) {
        throw std::system_error(errno, std::system_category(),
                                "IoEvent::setNoDelay");
    }
}
#endif

void IoEvent::setNonBlock(sockhdl fd, bool on) {
#if defined(__APPLE__) || defined(__linux__)
    int oldFlags = ::fcntl(fd, F_GETFL, 0);
    if (oldFlags < 0) {
        throw std::system_error(errno, std::system_category(),
                                "IoEvent::setNonBlock: Get");
    }

    int newFlags = 0;
    if (on) {
        newFlags = oldFlags | O_NONBLOCK;
    } else {
        newFlags = oldFlags | ~O_NONBLOCK;
    }

    if (::fcntl(fd, F_SETFL, newFlags) < 0) {
        throw std::system_error(errno, std::system_category(),
                                "IoEvent::setNonBlock: Set");
    }
#elif defined(_WIN32)
    u_long mode = 1;
    if (!on) {
        mode = 0;
    }

    if (ioctlsocket(fd, FIONBIO, &mode) != 0) {
        throw std::system_error(WSAGetLastError(), std::system_category(),
                                "IoEvent::setNonBlock");
    }
#endif
}

void IoEvent::setNoDelay(sockhdl fd) {
#if defined(__APPLE__) || defined(__linux__)
    int optVal = 1;
    socklen_t optLen = sizeof(optVal);

    int ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&optVal, optLen);
    if (ret != 0) {
        throw std::system_error(errno, std::system_category(),
                                "IoEvent::setNoDelay");
    }
#elif defined(_WIN32)
    BOOL optVal = TRUE;
    int optLen = sizeof(optVal);

    int ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&optVal, optLen);
    if (ret != 0) {
        throw std::system_error(WSAGetLastError(), std::system_category(),
                                "IoEvent::setNoDelay");
    }
#endif
}
}
