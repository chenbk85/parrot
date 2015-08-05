#ifndef __BASE_SYS_INC_LOGGERIMPL_H__
#define __BASE_SYS_INC_LOGGERIMPL_H__

#include "config.h"

namespace parrot
{
    class Logger
    {
      public:
        explicit Logger(const Config *cfg);
        ~Logger();

      public:
        void log();

      private:
        const Config *           _config;
    };
}


#endif
