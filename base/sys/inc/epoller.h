#ifndef __BASE_SYS_INC_EPOLLER_H__
#define __BASE_SYS_INC_EPOLLER_H__

namespace parrot
{
    class Epoller
    {
      public:
        Epoller();
        ~Epoller();

      public:
        void create();
        void waitFor();

      private:
        int _epollFd;
    };
}

#endif
