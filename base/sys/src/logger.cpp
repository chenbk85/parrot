#include "config.h"
#include "loggerThread.h"
#include "logger.h"
#include "loggerJob.h"

namespace parrot
{
Logger::Logger() : _logThread(nullptr), _config(nullptr)
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

void Logger::setConfig(Config* cfg) noexcept
{
    _config = cfg;
}

bool Logger::canLog(eLoggerLevel level) const noexcept
{
    if ((uint8_t)level < _config->_logLevel)
    {
        return false;
    }

    return true;
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

void Logger::log(eLoggerLevel level, int lineNo, const std::string& msg)
{
    std::unique_ptr<LoggerJob> job(new LoggerJob);
    job->doLog(level, lineNo, msg);
    _logThread->addJob(std::move(job));
}
}
