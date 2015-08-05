#include "logger.h"

namespace parrot
{
    Logger::Logger():
        _logThread(nullptr),
        _config(nullptr)
    {
    }

    Logger::~Logger()
    {
        stop();
    }

    Logger* Logger::instance() noexcept
    {
        static Logger logger;
        return &logger;
    }

    void Logger::setConfig(Config *cfg) noexcept
    {
        _config = cfg;
    }

    void Logger::start()
    {
        _logThread = new LoggerThread(_config);
        _logThread->start();
    }

    void Logger::stop()
    {
        if (_logThread == nullptr)
        {
            return;
        }

        _logThread->stop();
        delete _logThread;
        _logThread = nullptr;
    }

    void Logger::log(eLoggerLevel level, int lineNo, const char *fmt, ...)
    {
        if ((uint8_t)level < _config->_logLevel)
        {
            return;
        }

        va_list ap;
        va_start(ap, fmt);
        std::unique_ptr<va_list, void(*)(va_list*)> delHelper(
            &ap, [](va_list* v){ va_end(*v); });

        std::unique_ptr<LoggerJob> job (new LoggerJob);
        job->doLog(level, lineNo, fmt, ap);
        _logThread->addJob(std::move(job));
    }
}
