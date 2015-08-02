#ifndef __BASE_SYS_INC_EPOLLTRIGGER_H__
#define __BASE_SYS_INC_EPOLLTRIGGER_H__

namespace parrot
{
    class EpollTrigger
    {
      public:
        EpollTrigger();
        ~EpollTrigger();

      public:
        void create();
        void notify();
        void acknowledge();

      private:
        int _pipeFds[2];
    };
}

#endif
