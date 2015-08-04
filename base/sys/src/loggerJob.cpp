#include <stdio.h>
#include <thread>
#include <time.h> // C++11 doesn't have localtime_r, we have to use c version.
#include "loggerJob.h"

namespace parrot
{
    LoggerJob::LoggerJob():
        _logLen(0),
        _logBuff(new char[DEF_LOG_BUFF_LEN])
    {
    }

    LoggerJob::~LoggerJob()
    {
        _logBuff.reset(nullptr);
    }

    void LoggerJob::doLog(eLoggerLevel level, int lineNo, 
                          const char * fmt, va_list &args) noexcept
    {
        int maxLen = DEF_LOG_BUFF_LEN;

        while (true)
        {
            int usedLen = createHeader(level, lineNo);
            int ret = vsnprintf((char*)(_logBuff.get() + usedLen), 
                                maxLen - usedLen, fmt, args);
            if (ret >= maxLen - usedLen + 1) // Need to pad '\n'.
            {
                // Log is too long. Recreate.
                maxLen *= 4;
                _logBuff.reset(new char[maxLen]);
            }
            else
            {
                _logLen = usedLen + ret; // Minus the ending '\0'.
                if (_logBuff[_logLen - 1] != '\n')
                {
                    _logBuff[_logLen] = '\n';
                    _logBuff[_logLen + 1] = '\0';
                }
                break;
            }
        }
    }

    int LoggerJob::createHeader(eLoggerLevel level, int lineNo) noexcept
    {
        char dateStr[sizeof("2015-07-08 12:00:00")];
        struct tm tmRst;
        time_t now = time(nullptr);
        localtime_r(&now, &tmRst);
        strftime(dateStr, sizeof(dateStr), "%Y-%m-%d %H:%M:%S", &tmRst);

        return snprintf(_logBuff.get(), DEF_LOG_BUFF_LEN, 
                  "[%s][%-5s][%#014x][%05d] ",
                  dateStr,
                  getLevelStr(level),
                  std::this_thread::get_id(),
                  lineNo);
    }

    const char* LoggerJob::getLevelStr(eLoggerLevel level) noexcept
    {
        const char * str = nullptr;
        switch (level)
        {
            case eLL_Info:
                str = "INFO";
                break;

            case eLL_Debug:
                str = "DEBUG";
                break;

            case eLL_Warn:
                str = "WARN";
                break;

            case eLL_Error:
                str = "ERROR";
                break;

            case eLL_Fatal:
                str = "FATAL";
                break;

            default:
                break;
        }

        return str;
    }

    const char* LoggerJob::getLogBuff() const noexcept
    {
        return _logBuff.get();
    }

    uint32_t LoggerJob::getLogLen() const noexcept
    {
        return _logLen;
    }
}
