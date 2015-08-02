#ifndef __BASE_SYS_INC_IOEVENT_H__
#define __BASE_SYS_INC_IOEVENT_H__

namespace parrot
{
    enum IoAction
    {
        eIO_None,
        eIO_Read,
        eIO_Write,
        eIO_Remove
    }

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
        void setFd(int fd);
        
        // Retrive the associated fd.
        int getFd() const noexcept;

        // Retrive the epoll events.
        int getEpollEvents() const noexcept;

        // Set the epoll events.
        //
        // Params:
        // * events: EPOLLIN|EPOLLOUT ...
        void setEpollEvnets(int events) noexcept;

      public:
        // static help functions.
        // Make the fd non-blocking.
        static void setNonBlock();

        // Turn off nagle algorithm.
        static void setNoDelay();

        // Wrap for fcntl.
        //
        // Params:
        // * fd:    The file descriptor.
        // * flags: The file descripotrs, e.g., O_NONBLOCK ...
        static void manipulateFd(int fd, int flags);

      protected:
        virtual IoAction handleIoEvent() = 0;

      private:
        int _fd;
        int _epollEvents;
    };
}

#endif
