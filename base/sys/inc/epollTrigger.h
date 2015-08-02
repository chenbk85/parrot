#ifndef __BASE_SYS_INC_EPOLLTRIGGER_H__
#define __BASE_SYS_INC_EPOLLTRIGGER_H__

#include <memory>
#include "ioEvent.h"

namespace parrot
{
    class EpollTrigger : public IoEvent
    {
      public:
        EpollTrigger();
        ~EpollTrigger();
        EpollTrigger(const EpollTrigger&) = delete;
        EpollTrigger& operator=(const EpollTrigger&) = delete;

      public:
        void create();
        void trigger();
        void acknowledge();

      public:
        virtual IoAction handleIoEvent() override;

      private:
        int                              _pipeFds[2];
        std::unique_ptr<unsigned char[]> _buffer;
    };
}

#endif
