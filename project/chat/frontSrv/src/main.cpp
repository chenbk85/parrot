#include <memory>
#include "frontSrvMainThread.h"
#include "frontSrvConfig.h"

static void installConfig(chat::FrontSrvConfig &config)
{
    config._group = "xiyou";
    config._user = "leopold";
    config._lockFilePath = "./run/frontsrv.lock";
    config._frontThreadTimeout = 60;
    config._rpcThreadTimeout = 60;
    config._thisServer._isFront = true;
    config._thisServer._serverType = "frontSrv";
    config._thisServer._serverId = "frontSrv001";
    config._thisServer._publicIp = "10.24.100.202";
    config._thisServer._frontIp = "10.24.100.202";
    config._thisServer._frontPort = 9898;
    config._thisServer._rpcIp = "10.24.100.202";
    config._thisServer._rpcPort = 9899;
    config._logPath = "./log";
    config._logName = "frongSrv.log";
}

int main()
{
    chat::FrontSrvConfig config;
    installConfig(config);
    
    std::unique_ptr<chat::FrontSrvMainThread> mainThread(
        new chat::FrontSrvMainThread(&config));
    mainThread->start();
    
    return 0;
}
