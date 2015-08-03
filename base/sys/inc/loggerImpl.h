#ifndef __BASE_SYS_INC_LOGGERIMPL_H__
#define __BASE_SYS_INC_LOGGERIMPL_H__

#include "threadBase.h"
#include "config.h"

namespace parrot
{
    class LoggerImpl
    {
      public:
        explicit LoggerImpl(const Config *cfg);
        ~LoggerImpl();

      public:
        void log();

      protected:
        void run();

      private:
        const Config *           _config;
    };
}


#endif
