#ifndef __BASE_SYS_INC_EVENTNOTIFIER_H__
#define __BASE_SYS_INC_EVENTNOTIFIER_H__

namespace parrot
{
    class EventNotifier
    {
      public:
        EventNotifier();
        ~EventNotifier();

      public:
        void create();
        void notify();
        void acknowledge();

      private:
        int _pipeFds[2];
    };
}

#endif
