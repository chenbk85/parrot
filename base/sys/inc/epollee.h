#ifndef __BASE_SYS_INC_EPOLLEE_H__
#define __BASE_SYS_INC_EPOLLEE_H__

namespace parrot
{
    class Epollee
    {
      public:
        Epollee();
        ~Epollee();
        Epollee(const Epollee&) = delete;
        Epollee &operator=(const Epollee&) = delete;

      public:

      private:
        int _fd;
    };
}

#endif
