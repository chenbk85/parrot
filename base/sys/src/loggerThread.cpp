#include <cstdio> // For rename.
#include <string>

#include "config.h"
#include "macroFuncs.h"
#include "loggerThread.h"
#include "loggerJob.h"
#include "ioEvent.h"
#include "epoll.h"
#include "kqueue.h"
#include "codes.h"
#include "simpleEventNotifier.h"

namespace parrot {
LoggerThread::LoggerThread(const Config *cfg)
    : ThreadBase(), _jobListLock(), _logJobList(), _logFullPath(),
      _currFileSize(0), _fileStream(),
#if defined(__linux__)
      _notifier(new Epoll(1)),
#elif defined(__APPLE__)
      _notifier(new Kqueue(1)),
#elif defined(_WIN32)
      _notifier(new SimpleEventNotifier())
#endif
      _config(cfg) {
    if ((_config->_logPath).empty() || (_config->_logName).empty()) {
        PARROT_ASSERT(false);
    }

    auto len = (_config->_logPath).length();
    if ((_config->_logPath)[len - 1] == '/') {
        _logFullPath = _config->_logPath + _config->_logName;
    } else {
        _logFullPath = _config->_logPath + '/' + _config->_logName;
    }
}

LoggerThread::~LoggerThread() = default;

void LoggerThread::beforeStart() {
    _notifier->create();
}

void LoggerThread::addJob(std::unique_ptr<LoggerJob> &&job) noexcept {
    _jobListLock.lock();
    _logJobList.push_back(std::move(job));
    _jobListLock.unlock();

    _notifier->stopWaiting();
}

void LoggerThread::stop() {
    _notifier->stopWaiting();
    ThreadBase::stop();
    _fileStream.close();
}

void LoggerThread::createLog() {
    if (_fileStream.is_open()) {
        return;
    }

    _fileStream.open(_logFullPath.c_str(), std::ios::out | std::ios::app);

    if (!_fileStream.is_open()) {
        throw std::system_error(static_cast<int>(eCodes::ERR_FileOpen),
                                ParrotCategory(), "LoggerThread::createLog");
    }

    _currFileSize = static_cast<uint64_t>(_fileStream.tellp());
}

void LoggerThread::writeToLog(JobListType &jobList) {
    for (auto &j : jobList) {
        _fileStream << j->getLogBuff();

        if (_fileStream.fail()) {
            throw std::system_error(static_cast<int>(eCodes::ERR_FileWrite),
                                    ParrotCategory(),
                                    "LoggerThread::createLog");
        }
        _currFileSize += j->getLogLen();
    }

    jobList.clear();
}

void LoggerThread::rotateLog() {
    if (_currFileSize < _config->_logSize || _config->_rotateNum == 0) {
        return;
    }

    std::string oldName = "";
    std::string newName = "";

    _fileStream.close();
    _currFileSize = 0u;

    for (auto i = _config->_rotateNum; i > 0u; --i) {
        if (i - 1 == 0) {
            oldName = _logFullPath;
        } else {
            oldName = _logFullPath + '.' + std::to_string(i - 1);
        }

        newName = _logFullPath + '.' + std::to_string(i);
        std::rename(oldName.c_str(), newName.c_str());
    }
}

void LoggerThread::processJob() {
    JobListType jobList;
    _jobListLock.lock();
    jobList = std::move(_logJobList);
    _jobListLock.unlock();

    if (jobList.empty()) {
        return;
    }

    createLog();
    writeToLog(jobList);
    rotateLog();
}

void LoggerThread::run() {
    IoEvent *ev = nullptr;
    uint32_t ret = 0;

    while (!isStopped()) {
        ret = _notifier->waitIoEvents(-1);

        for (auto i = 0u; i < ret; ++i) {
            ev = _notifier->getIoEvent(i);
            ev->handleIoEvent();
        }

        processJob();
    }
}
}
