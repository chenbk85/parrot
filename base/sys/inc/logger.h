#ifndef __BASE_SYS_INC_LOGGERIMPL_H__
#define __BASE_SYS_INC_LOGGERIMPL_H__

#include <cstdint>
#include <string>
#include <sstream>
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
        bool canLog(eLoggerLevel level) const noexcept;
        void start();
        void stop();
        void log(eLoggerLevel level, int lineNo, const std::string &msg);

      private:
        LoggerThread *           _logThread;
        const Config *           _config;
    };
}

#define LOG_INFO(msg)                                   \
    do                                                  \
    {                                                   \
        if (parrot::Logger::instance()->canLog(         \
                parrot::eLoggerLevel::Info))            \
        {                                               \
            ostringstream ostr;                         \
            ostr << msg;                                \
            parrot::Logger::instance()->log(            \
                parrot::eLoggerLevel::Info, __LINE__,   \
                ostr.str());                            \
        }                                               \
    }                                                   \
    while (false)

#define LOG_DEBUG(msg)                                  \
    do                                                  \
    {                                                   \
        if (parrot::Logger::instance()->canLog(         \
                parrot::eLoggerLevel::Debug))           \
        {                                               \
            ostringstream ostr;                         \
            ostr << msg;                                \
            parrot::Logger::instance()->log(            \
                parrot::eLoggerLevel::Debug, __LINE__,  \
                ostr.str());                            \
        }                                               \
    }                                                   \
    while (false)

#define LOG_WARN(msg)                                   \
    do                                                  \
    {                                                   \
        if (parrot::Logger::instance()->canLog(         \
                parrot::eLoggerLevel::Warn))            \
        {                                               \
            ostringstream ostr;                         \
            ostr << msg;                                \
            parrot::Logger::instance()->log(            \
                parrot::eLoggerLevel::Warn, __LINE__,   \
                ostr.str());                            \
        }                                               \
    }                                                   \
    while (false)

#define LOG_ERROR(msg)                                      \
        do                                                  \
        {                                                   \
            if (parrot::Logger::instance()->canLog(         \
                    parrot::eLoggerLevel::Error))           \
            {                                               \
                ostringstream ostr;                         \
                ostr << msg;                                \
                parrot::Logger::instance()->log(            \
                    parrot::eLoggerLevel::Error, __LINE__,  \
                    ostr.str());                            \
            }                                               \
        }                                                   \
        while(false)

#define LOG_FATAL(msg)                                      \
        do                                                  \
        {                                                   \
            if (parrot::Logger::instance()->canLog(         \
                    parrot::eLoggerLevel::Fatal))           \
            {                                               \
                ostringstream ostr;                         \
                ostr << msg;                                \
                parrot::Logger::instance()->log(            \
                    parrot::eLoggerLevel::Fatal, __LINE__,  \
                    ostr.str());                            \
            }                                               \
        }                                                   \
        while (false)

#endif
