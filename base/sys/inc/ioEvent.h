#ifndef __BASE_SYS_INC_IOEVENT_H__
#define __BASE_SYS_INC_IOEVENT_H__

#include <cstdint>

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
        IoEvent() noexcept;
        virtual ~IoEvent();
        IoEvent(const IoEvent&) = delete;
        IoEvent &operator=(const IoEvent&) = delete;

      public:
        // Associate IoEvent with the fd.
        //
        // Params:
        // * fd: File descriptor.
        void setFd(int fd) noexcept;
        
        // Retrive the associated fd.
        int getFd() const noexcept;

        // Default to read or write.
        void setAction(eIoAction act) noexcept;

        // Retrive current action.
        eIoAction getAction() const noexcept;

        // Is the event error?
        bool isError() const noexcept;
        
        // Is the peer closed the socket?
        bool isEof() const noexcept;

        // Implement this function to handle epoll event.
        bool isReadAvail() const noexcept;

        // Implement this function to handle epoll event.
        bool isWriteAvail() const noexcept;

        // Implement this function to handle epoll event.
        virtual eIoAction handleIoEvent() = 0;

        virtual void close() noexcept;

      public:
        uint32_t send(const char* buff, uint32_t buffLen);
        uint32_t recv(char* buff, uint32_t buffLen);

      protected:
        // Help function to mark the event to read.
        void setIoRead() noexcept;

        // Help function to mark the event to write.
        void setIoWrite() noexcept;

        int getFilter() const noexcept;

        void setFilter(int filter) noexcept;

        void setFlags(int flags) noexcept;

        int getFlags() const noexcept;

      public:
        // static help functions.
        
        // Make the fd non-blocking.
        //
        // Params:
        // * fd: The target file descriptor.
        static void setNonBlock(int fd);

        // Turn off nagle algorithm.
        //
        // Params:
        // * fd: The target file descriptor.
        static void setNoDelay(int fd);

        // Wrap fcntl.
        //
        // Params:
        // * fd:    The file descriptor.
        // * flags: The file descripotrs, e.g., O_NONBLOCK ...
        static void manipulateFd(int fd, int flags);

      protected:
        int                                 _fd;
        int                                 _filter;
        int                                 _flags;
        eIoAction                           _action;
    };
}

#endif
