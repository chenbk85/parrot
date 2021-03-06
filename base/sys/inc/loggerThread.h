#ifndef __BASE_SYS_INC_LOGGERTHREAD_H__
#define __BASE_SYS_INC_LOGGERTHREAD_H__

#include <mutex>
#include <list>
#include <string>
#include <memory>
#include <fstream>

#include "threadBase.h"

namespace parrot
{
class LoggerJob;
class EventNotifier;
struct Config;

class LoggerThread : public ThreadBase
{
    using JobListType = std::list<std::unique_ptr<LoggerJob>>;

  public:
    explicit LoggerThread(const Config* cfg);
    virtual ~LoggerThread();
    LoggerThread(const LoggerThread&) = delete;
    LoggerThread& operator=(const LoggerThread&) = delete;
    LoggerThread(LoggerThread&&) = delete;
    LoggerThread& operator=(LoggerThread&&) = delete;

  public:
    void beforeStart() override;
    void addJob(std::unique_ptr<LoggerJob>&& job) noexcept;
    void stop() override;

  protected:
    void run() override;

  private:
    void createLog();
    void rotateLog();
    void writeToLog(JobListType& jobList);
    void processJob();

  private:
    std::mutex _jobListLock;
    JobListType _logJobList;
    std::string _logFullPath;
    uint64_t _currFileSize;
    std::ofstream _fileStream;
    std::unique_ptr<EventNotifier> _notifier;
    const Config* _config;
};
}

#endif
