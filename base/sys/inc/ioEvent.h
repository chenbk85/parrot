#ifndef __BASE_SYS_INC_IOEVENT_H__
#define __BASE_SYS_INC_IOEVENT_H__

namespace parrot
{
    enum IoAction
    {
        eIO_None,
        eIO_Read,
        eIO_Write,
        eIO_Remove,
        eIO_Close
    }

    class IoEvent
    {
      public:
        IoEvent();
        ~IoEvent();
        IoEvent(const IoEvent&) = delete;
        IoEvent &operator=(const IoEvent&) = delete;

      public:
        void setNonBlock();
        void setNoDelay();
        int getFd() const noexcept;

      protected:
        virtual IoAction handleIoEvent() = 0;

      private:
        int _fd;
    };
}

#endif
