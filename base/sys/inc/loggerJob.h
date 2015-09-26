#ifndef __BASE_SYS_INC_LOGGERJOB_H__
#define __BASE_SYS_INC_LOGGERJOB_H__

#include <stdarg.h>
#include <memory>
#include <thread>
#include <cstdint>

namespace parrot {
enum class eLoggerLevel : uint8_t {
    Info = 0,
    Debug = 1,
    Warn = 2,
    Error = 3,
    Fatal = 4
};

class LoggerJob {
    enum { DEF_HEADER_LEN = 56 };

  public:
    LoggerJob() noexcept;
    ~LoggerJob();
    LoggerJob(const LoggerJob &) = delete;
    LoggerJob &operator=(const LoggerJob &) = delete;
    LoggerJob(LoggerJob &&job) noexcept;

  public:
    void doLog(eLoggerLevel level, int lineNo, const std::string &msg) noexcept;
    int createHeader(eLoggerLevel level, int lineNo) noexcept;
    const char *getLevelStr(eLoggerLevel level) noexcept;
    const char *getLogBuff() const noexcept;
    uint32_t getLogLen() const noexcept;

  private:
    uint32_t _logLen;
    std::hash<std::thread::id> _hasher;
    std::unique_ptr<char[]> _logBuff;
};
}

#endif
