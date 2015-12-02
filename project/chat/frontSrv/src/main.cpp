#include <memory>
#include "frontSrvMainThread.h"
#include "frontSrvConfig.h"
#include "urlParser.h"

static void installConfig(chat::FrontSrvConfig& config)
{
    config._group                  = "xiyou";
    config._user                   = "leopold";
    config._lockFilePath           = "./run/frontsrv.lock";
    config._frontThreadTimeout     = 60;
    config._rpcThreadTimeout       = 60;
    config._thisServer._isFront    = true;
    config._thisServer._serverType = "frontSrv";
    config._thisServer._serverId   = "frontSrv001";
    config._thisServer._publicIp   = "10.24.100.202";
    config._thisServer._frontIp    = "10.24.100.202";
    config._thisServer._frontPort  = 9898;
    config._thisServer._rpcIp      = "10.24.100.202";
    config._thisServer._rpcPort    = 9899;
    config._thisServer._frontWsUrl = "ws://10.24.100.202:9898";
    config._thisServer._rpcWsUrl   = "ws://10.24.100.202:9899";
    config._frontThreadPoolSize    = 1;
    config._logicThreadPoolSize    = 1;
    config._logPath                = "./log";
    config._logName                = "frongSrv.log";
}

static void postProcessConfig(chat::FrontSrvConfig& config)
{
    config._thisServer._frontWsUrlInfo =
        parrot::UrlParser::parse(config._thisServer._frontWsUrl);

    config._thisServer._rpcWsUrlInfo =
        parrot::UrlParser::parse(config._thisServer._rpcWsUrl);
}

int main()
{
    chat::FrontSrvConfig config;
    installConfig(config);
    postProcessConfig(config);

    std::unique_ptr<chat::FrontSrvMainThread> mainThread(
        new chat::FrontSrvMainThread(&config));
    mainThread->start();

    return 0;
}
