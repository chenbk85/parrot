#ifndef __PROJECT_CHAT_FRONTSRV_INC_FRONTSRVMAINTHREAD_H__
#define __PROJECT_CHAT_FRONTSRV_INC_FRONTSRVMAINTHREAD_H__

#include "mainThread.h"
#include "threadPool.h"
#include "frontSrvLogicThread.h"

namespace chat
{
struct FrontSrvConfig;    
    
class FrontSrvMainThread : public parrot::MainThread
{
  public:
    explicit FrontSrvMainThread(const FrontSrvConfig* cfg);

  protected:
    void createUserThreads() override;
    
  protected:
    parrot::ThreadPool<FrontSrvLogicThread> _logicThreadPool;
};

}
#endif