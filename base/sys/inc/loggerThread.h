#ifndef __BASE_SYS_INC_LOGGERTHREAD_H__
#define __BASE_SYS_INC_LOGGERTHREAD_H__

#include <mutex>
#include <list>

#include "config.h"
#include "threadBase.h"

namespace parrot
{
    class LoggerJob;

    class LoggerThread: public ThreadBase
    {
      public:
        LoggerThread(const Config *cfg);
        ~LoggerThread();
        LoggerThread(const LoggerThread &) = delete;
        LoggerThread& operator=(const LoggerThread&) = delete;

      public:
        void init();
        void addJob(std::unique_ptr<LoggerJob> &&job);

      protected:
        void run() override;

      private:
        void createLog();
        void rotateLog();
        void writeToLog();

      private:
        std::mutex                                 _jobListLock;
        std::list<std::unique_ptr<LoggerJob>>      _logJobList;
        const Config *                             _config;
    };
}

#endif
