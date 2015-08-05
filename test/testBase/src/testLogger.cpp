#include <iostream>
#include <memory>
#include <stdarg.h>
#include <stdio.h>

#include "config.h"
#include "logger.h"

#include <thread>
#include <chrono>

#include <sys/types.h>
#include <sys/stat.h>

using namespace std;



int main()
{
    umask(0133);

    std::unique_ptr<parrot::Config> cfg (new parrot::Config());
    cfg->_logPath = ".";
    cfg->_logName = "test.log";

    parrot::Logger *logger = parrot::Logger::instance();
    logger->setConfig(cfg.get());
    logger->start();
    for (int i = 0; i < 20500; ++i)
    {
        LOG_DEBUG("Main %d", i);
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
    logger->stop();
}
