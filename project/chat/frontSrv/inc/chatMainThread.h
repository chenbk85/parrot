#ifndef __PROJECT_CHAT_FRONTSRV_INC_CHATMAINTHREAD_H__
#define __PROJECT_CHAT_FRONTSRV_INC_CHATMAINTHREAD_H__

#include "mainThread.h"
#include "threadPool.h"

namespace chat
{
class ChatMainThread : public parrot::MainThread
{
  public:
    ChatMainThread();

  protected:
    void createUserThreads() override;
    
  protected:
    parrot::ThreadPool<LogicThread> _logicThreadPool;
};

}
#endif
