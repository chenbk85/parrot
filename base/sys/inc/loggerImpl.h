#ifndef __BASE_SYS_INC_LOGGERIMPL_H__
#define __BASE_SYS_INC_LOGGERIMPL_H__

#include "threadBase.h"
#include "config.h"

namespace parrot
{
    enum eLoggerLevel
    {
        eLL_Info,
        eLL_Debug,
        eLL_Warn,
        eLL_Error,
        eLL_Fatal
    };

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
