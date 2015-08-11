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
        IoEnd,
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
        int getEpollEvents() const noexcept;

        // Set the epoll events.
        //
        // Params:
        // * events: EPOLLIN|EPOLLOUT ...
        void setEpollEvents(int events) noexcept;

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
        int _fd;
        int _epollEvents;
    };
}

#endif
