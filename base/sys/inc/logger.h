#ifndef __BASE_SYS_INC_LOGGERIMPL_H__
#define __BASE_SYS_INC_LOGGERIMPL_H__

#include <cstdint>
#include "loggerJob.h"

namespace parrot
{
    struct Config;
    class LoggerThread;

    class Logger
    {
      public:
        Logger();
        ~Logger();

      public:
        static Logger* instance() noexcept;
        void setConfig(Config *cfg) noexcept;
        void start();
        void stop();
        void log(eLoggerLevel level, int lineNo, const char *fmt, ...);

      private:
        LoggerThread *           _logThread;
        const Config *           _config;
    };
}

#define LOG_INFO(fmt, ...)                                              \
    do                                                                  \
    {                                                                   \
        parrot::Logger::instance()->log(                                \
            parrot::eLoggerLevel::Info, __LINE__, fmt, ##__VA_ARGS__);  \
    } while (false)

#define LOG_DEBUG(fmt, ...)                                             \
    do                                                                  \
    {                                                                   \
        parrot::Logger::instance()->log(                                \
            parrot::eLoggerLevel::Debug, __LINE__, fmt, ##__VA_ARGS__); \
    } while (false)

#define LOG_WARN(fmt, ...)                                              \
    do                                                                  \
    {                                                                   \
        parrot::Logger::instance()->log(                                \
            parrot::eLoggerLevel::Warn, __LINE__, fmt, ##__VA_ARGS__);  \
    } while (false)

#define LOG_ERROR(fmt, ...)                                             \
    do                                                                  \
    {                                                                   \
        parrot::Logger::instance()->log(                                \
            parrot::eLoggerLevel::Error, __LINE__, fmt, ##__VA_ARGS__); \
    } while (false)

#define LOG_FATAL(fmt, ...)                                             \
    do                                                                  \
    {                                                                   \
        parrot::Logger::instance()->log(                                \
            parrot::eLoggerLevel::Fatal, __LINE__, fmt, ##__VA_ARGS__); \
    } while (false)

#endif
