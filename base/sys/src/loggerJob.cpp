#include <stdio.h>
#include <thread>
#include <time.h> // C++11 doesn't have localtime_r, we have to use raw version.
#include "loggerJob.h"

namespace parrot
{
    LoggerJob::LoggerJob() noexcept:
        _logLen(0),
        _hasher(),
        _logBuff(new char[DEF_LOG_BUFF_LEN])
    {
        static_assert(DEF_LOG_BUFF_LEN >= 256, "Minimal log buff len");
    }

    LoggerJob::LoggerJob(LoggerJob &&job) noexcept:
        _logLen(job._logLen),
        _hasher(std::move(job._hasher)),
        _logBuff(std::move(job._logBuff))
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
                    _logBuff[++_logLen] = '\0';
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
                        "[%s][%-5s][%#018lx][%05d] ",
                        dateStr,
                        getLevelStr(level),
                        _hasher(std::this_thread::get_id()),
                        lineNo);
    }

    const char* LoggerJob::getLevelStr(eLoggerLevel level) noexcept
    {
        const char * str = nullptr;
        switch (level)
        {
            case eLoggerLevel::Info:
                str = "INFO";
                break;

            case eLoggerLevel::Debug:
                str = "DEBUG";
                break;

            case eLoggerLevel::Warn:
                str = "WARN";
                break;

            case eLoggerLevel::Error:
                str = "ERROR";
                break;

            case eLoggerLevel::Fatal:
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
