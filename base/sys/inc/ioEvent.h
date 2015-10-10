#ifndef __BASE_SYS_INC_IOEVENT_H__
#define __BASE_SYS_INC_IOEVENT_H__

#include <cstdint>
#include <string>

#include "codes.h"
#include "unifyPlatDef.h"

namespace parrot
{
enum class eIoAction : uint8_t
{
    None,
    Read,
    Write,
    Remove
};

class IoEvent
{
#if defined(__linux__)
    friend class EpollImpl;
#elif defined(__APPLE__)
    friend class KqueueImpl;
#endif

  public:
    IoEvent();
    virtual ~IoEvent();
    IoEvent(const IoEvent&) = delete;
    IoEvent& operator=(const IoEvent&) = delete;

  public:
    // Associate IoEvent with the fd.
    //
    // Params:
    // * fd: File descriptor.
    void setFd(int fd);

    // Retrive the associated fd.
    int getFd() const;

    // Default to read or write.
    void setAction(eIoAction act);

    // Retrive current action.
    eIoAction getAction() const;

    // Is the event error?
    virtual bool isError() const;

    // Is the peer closed the socket?
    virtual bool isEof() const;

    // Implement this function to handle epoll event.
    virtual bool isReadAvail() const;

    // Implement this function to handle epoll event.
    virtual bool isWriteAvail() const;

    // Implement this function to handle epoll event.
    virtual eIoAction handleIoEvent() = 0;

    virtual void close();

    void setRemoteAddr(const std::string& ip);

    void setRemoteAddr(std::string&& ip);

    const std::string& getRemoteAddr() const;

    void setUniqueKey(uint64_t key);

    uint64_t getUniqueKey() const;

  protected:
    // Help function to mark the event to read.
    void setIoRead();

    // Help function to mark the event to write.
    void setIoWrite();

    int getFilter() const;

    void setFilter(int filter);

    void setFlags(int flags);

    int getFlags() const;

  public:
#if defined(__APPLE__) || defined(__linux__)
    virtual eCodes send(const char* buff, uint32_t buffLen, uint32_t& sentLen);
    virtual eCodes recv(char* buff, uint32_t buffLen, uint32_t& rcvdLen);
#endif
  protected:
    int _fd;
    int _filter;
    int _flags;
    uint64_t _uniqueKey;
    eIoAction _action;
    std::string _remoteIp;
};
}

#endif
