#include <thread>
#include <stdio.h>
#include <time.h> // C++11 doesn't have localtime_r, we have to use raw version.

#include "loggerJob.h"

#include <iostream>

namespace parrot
{
LoggerJob::LoggerJob() noexcept : _logLen(0), _hasher(), _logBuff()
{
}

LoggerJob::LoggerJob(LoggerJob&& job) noexcept
    : _logLen(job._logLen),
      _hasher(std::move(job._hasher)),
      _logBuff(std::move(job._logBuff))
{
}

LoggerJob::~LoggerJob()
{
    _logBuff.reset(nullptr);
}

void LoggerJob::doLog(eLoggerLevel level, int lineNo,
                      const std::string& msg) noexcept
{
    using namespace std;

    int buffLen = DEF_HEADER_LEN + msg.length() + 2; // 2 is for \n\0
    _logBuff.reset(new char[buffLen]);

    char dateStr[sizeof("2015-07-08 12:00:00")];
    struct tm tmRst;
    time_t now = time(nullptr);
    localtime_r(&now, &tmRst);
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d %H:%M:%S", &tmRst);

    auto len =
        snprintf(_logBuff.get(), buffLen, "[%s][%-5s][%#018lx][%05d] %s",
                 dateStr, getLevelStr(level),
                 _hasher(std::this_thread::get_id()), lineNo, msg.c_str());

    if (_logBuff[len - 1] != '\n')
    {
        _logBuff[len] = '\n';
        _logBuff[len + 1] = '\0';
        _logLen = len + 1;
    }
    else
    {
        _logLen = len;
    }
}

const char* LoggerJob::getLevelStr(eLoggerLevel level) noexcept
{
    const char* str = nullptr;
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
