#ifndef __BASE_SYS_INC_IOEVENT_H__
#define __BASE_SYS_INC_IOEVENT_H__

#include <cstdint>
#include <string>

#include "codes.h"
#include "unifyPlatDef.h"

#include "sysDefinitions.h"

namespace parrot
{
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

    // Retrive the default action of the event.
    virtual eIoAction getDefaultAction() const;

    // Set next action.
    void setNextAction(eIoAction act);

    // Get next action.
    eIoAction getNextAction() const;

    // Set curr action.
    void setCurrAction(eIoAction act);

    // Get curr action.
    eIoAction getCurrAction() const;

    // Set notified action.
    void setNotifiedAction(eIoAction act);

    // Get notified action.
    eIoAction getNotifiedAction() const;

    bool sameAction() const;

    void setError(bool isErr);
    // Is the event error?
    bool isError() const;

    void setEof(bool isEof);
    // Is the peer closed the socket?
    bool isEof() const;

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

  public:
#if defined(__APPLE__) || defined(__linux__)
    virtual eCodes send(const char* buff, uint32_t buffLen, uint32_t& sentLen);
    virtual eCodes recv(char* buff, uint32_t buffLen, uint32_t& rcvdLen);
#endif
  protected:
    int _fd;
    uint64_t _uniqueKey;
    eIoAction _nextAction;
    // Kqueue read event and write event shares one IoEvent, make
    // sure only one of read & write is available, or here will
    // be a bug. Because they will both update this flag.
    eIoAction _currAction;
    eIoAction _notifiedAction;
    bool _isError;
    bool _isEof;
    std::string _remoteIp;
};
}

#endif
