#ifndef __BASE_SYS_INC_LOGGERJOB_H__
#define __BASE_SYS_INC_LOGGERJOB_H__

#include <stdarg.h>
#include <memory>

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

    class LoggerJob
    {
        enum
        {
            DEF_LOG_BUFF_LEN = 1024
        };

      public:
        LoggerJob();
        ~LoggerJob();
        LoggerJob(const LoggerJob&) = delete;
        LoggerJob& operator=(const LoggerJob&) = delete;

      public:
        void doLog(eLoggerLevel level, int lineNo, 
                   const char * fmt, va_list &args) noexcept;
        int createHeader(eLoggerLevel level, int lineNo) noexcept;
        const char* getLevelStr(eLoggerLevel level) noexcept;
        const char * getLogBuff() const noexcept;
        uint32_t getLogLen() const noexcept;
        
      private:
        uint32_t                           _logLen;
        std::unique_ptr<char[]>   _logBuff;
    };
}

#endif
