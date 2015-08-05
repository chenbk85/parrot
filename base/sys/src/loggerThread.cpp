#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>

#include "loggerThread.h"
#include "loggerJob.h"
#include "epoll.h"

namespace parrot
{
    LoggerThread::LoggerThread(const Config *cfg):
        ThreadBase(),
        _jobListLock(),
        _logJobList(),
        _logFullPath(),
        _currFileSize(0),
        _epoll(new Epoll(1)),
        _config(cfg)
    {
        if ((_config->_logPath).empty() || (_config->_logName).empty())
        {
            assert(0);
        }

        auto len = (_config->_logPath).length();
        if ((_config->_logPath)[len - 1] == '/')
        {
            _logFullPath = _config->_logPath + _config->_logName;
        }
        else
        {
            _logFullPath = _config->_logPath + '/'  + _config->_logName;
        }
    }

    LoggerThread::~LoggerThread()
    {

    }

    void LoggerThread::init()
    {
        _epoll->create();
    }

    void LoggerThread::addJob(std::unique_ptr<LoggerJob> &&job) noexcept
    {
        _jobListLock.lock();
        _logJobList.push_back(std::move(job));
        _jobListLock.unlock();

        _epoll->stopWaiting();
    }

    void LoggerThread::stop()
    {
        _epoll->stopWaiting();
        ThreadBase::stop();

        ::close(_logFd);
        _logFd = -1;
    }

    void LoggerThread::createLog()
    {
        if (_logFd >= 0)
        {
            return;
        }

        _logFd = ::open(_logFullPath.c_str(), O_WRONLY|O_CREAT|O_APPEND);

        if (_logFd < 0)
        {
            throw std::system_error(errno, std::system_category(), 
                                    "LoggerThread::createLog:open");
        }

        struct stat st;
        if (fstat(_logFd, &st) < 0)
        {
            throw std::system_error(errno, std::system_category(), 
                                    "LoggerThread::createLog:fstat");            
        }
        _currFileSize = st.st_size;
    }

    void LoggerThread::writeToLog(JobListType &jobList)
    {
        for (auto &j: jobList)
        {
            auto buff = j->getLogBuff();
            auto len = j->getLogLen();
            auto needWrite = len;
            int ret = 0;

            while (true)
            {
                ret = ::write(_logFd, buff, needWrite);
                if (ret >= 0)
                {
                    needWrite -= ret;
                    if (needWrite == 0)
                    {
                        break;
                    }
                    else
                    {
                        buff = buff + len - needWrite;
                    }
                }
                else 
                {
                    if (errno == EINTR)
                    {
                        continue;
                    }

                    throw std::system_error(errno, std::system_category(), 
                                            "LoggerThread::writeToLog:write");
                }
            }
        }

        jobList.clear();
    }

    void LoggerThread::rotateLog()
    {
        if (_currFileSize < _config->_logSize || _config->_rotateNum == 0)
        {
            return;
        }

        std::string oldName = "";
        std::string newName = "";

        ::close(_logFd);
        _logFd = -1;
        _currFileSize = 0u;

        for (auto i = _config->_rotateNum; i > 0u; --i)
        {
            if (i - 1 == 0)
            {
                oldName = _logFullPath;
            }
            else
            {
                oldName = _logFullPath + '.' + std::to_string(i - 1);
            }

            newName = _logFullPath + '.' + std::to_string(i);
            ::rename(oldName.c_str(), newName.c_str());
        }
    }

    void LoggerThread::processJob()
    {
        JobListType jobList;
        _jobListLock.lock();
        jobList = std::move(_logJobList);
        _jobListLock.unlock();

        if (jobList.empty())
        {
            return;
        }
        
        createLog();
        writeToLog(jobList);
        rotateLog();
    }

    void LoggerThread::run()
    {
        IoEvent *ev = nullptr;
        int ret = 0;

        while (!isStopped())
        {
            ret = _epoll->waitIoEvents(-1);
            if (ret <= 0)
            {
                continue;
            }

            for (int i = 0; i < ret; ++i)
            {
                ev = _epoll->getIoEvent(i);
                ev->handleIoEvent();
            }

            processJob();
        }
    }
}