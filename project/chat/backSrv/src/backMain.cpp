#include <memory>
#include "backSrvMainThread.h"
#include "backSrvConfig.h"
#include "urlParser.h"

static void installConfig(chat::BackSrvConfig& config)
{
    config._group                  = "leopold";
    config._user                   = "leopold";
    config._lockFilePath           = "./run/backsrv.lock";
    config._frontThreadTimeout     = 60;
    config._rpcThreadTimeout       = 60;
    config._thisServer._isFront    = false;
    config._thisServer._serverType = "backSrv";
    config._thisServer._serverId   = "backSrv001";
    config._thisServer._rpcIp      = "10.24.100.202";
    config._thisServer._rpcPort    = 9899;
    config._thisServer._rpcWsUrl   = "ws://10.24.100.202:9899";
    config._frontThreadPoolSize    = 1;
    config._logicThreadPoolSize    = 1;
    config._logPath                = "./log";
    config._logName                = "backSrv.log";
}

static void postProcessConfig(chat::BackSrvConfig& config)
{
    config._thisServer._frontWsUrlInfo =
        parrot::UrlParser::parse(config._thisServer._frontWsUrl);
}

int main()
{
    chat::BackSrvConfig config;
    installConfig(config);
    postProcessConfig(config);

    std::unique_ptr<chat::BackSrvMainThread> mainThread(
        new chat::BackSrvMainThread(&config));
    mainThread->start();
    return 0;
}

