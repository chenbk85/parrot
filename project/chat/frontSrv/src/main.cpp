#include <memory>
#include "frontSrvMainThread.h"
#include "frontSrvConfig.h"

int main()
{
    chat::FrontSrvConfig config;
    std::unique_ptr<chat::FrontSrvMainThread> mainThread(
        new chat::FrontSrvMainThread(&config));

    mainThread->start();
    return 0;
}
