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
        ReadHup,
        IoEOF,
        Error,
        Remove,
        TotalFlags
     };

    class IoEvent
    {
      public:
        IoEvent() noexcept;
        ~IoEvent();
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

        // Retrive the epoll events.
        const EnumClassBitset<eIoAction> &getActions() const noexcept;

        // Help function to mark the event to read.
        void setIoRead() noexcept;

        // Help function to mark the event to write.
        void setIoWrite() noexcept;

        int getFilter() const noexcept;

        void setFilter(int filter) noexcept;

        void setFlags(int flags) noexcept;

        int getFlags() const noexcept;

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

      private:
        int                                 _fd;
        int                                 _filter;
        int                                 _flags;
        EnumClassBitset<eIoAction>          _actions;
    };
}

#endif
