#include <iostream>
#include <memory>

#include "config.h"
#include "logger.h"

#include <thread>
#include <chrono>

#if defined(__linux) || defined(__APPLE__)
#include <sys/stat.h>
#endif

using namespace std;



int main()
{
#if defined(__linux) || defined(__APPLE__)
    umask(0133);
#endif

    std::unique_ptr<parrot::Config> cfg (new parrot::Config());
    cfg->_logPath = ".";
    cfg->_logName = "test.log";

    parrot::Logger *logger = parrot::Logger::instance();
    logger->setConfig(cfg.get());
    logger->start();
    for (int i = 0; i < 200000000; ++i)
    {
        LOG_DEBUG("aaa" << "1111" << 2222 << "bbb\n");
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
    logger->stop();
}
