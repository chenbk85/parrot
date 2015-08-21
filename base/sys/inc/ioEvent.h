#ifndef __BASE_SYS_INC_IOEVENT_H__
#define __BASE_SYS_INC_IOEVENT_H__

#include <cstdint>
#include "unifyPlatDef.h"

namespace parrot
{
    enum class eIoAction : uint16_t
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
        IoEvent &operator=(const IoEvent&) = delete;

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
        uint32_t send(const char* buff, uint32_t buffLen);
        uint32_t recv(char* buff, uint32_t buffLen);
#endif
      public:
        // static help functions.
        
        // Make the fd non-blocking.
        //
        // Params:
        // * fd: The target file descriptor.
        // * on: If true, make the fd nonblock. or make it block.
        static void setNonBlock(sockhdl fd, bool on = true);

        // Turn off nagle algorithm.
        //
        // Params:
        // * fd: The target file descriptor.
        static void setNoDelay(sockhdl fd);

#if defined(_WIN32)
        // Set the socket exclusive.
        //
        // Params:
        // * fd: The target file descriptor.
        static void setExclusiveAddr(sockhdl fd);
#elif defined(__linux__) || defined(__APPLE__)
        // Do not use reuse addr in Windows.
        //
        // Params:
        // * fd: The target file descriptor.
        static void setReuseAddr(sockhdl fd);
#endif

      protected:
        int                                 _fd;
        int                                 _filter;
        int                                 _flags;
        eIoAction                           _action;
    };
}

#endif
