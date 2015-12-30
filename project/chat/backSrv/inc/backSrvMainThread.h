#ifndef __PROJECT_CHAT_BACKSRV_INC_BACKSRVMAINTHREAD_H__
#define __PROJECT_CHAT_BACKSRV_INC_BACKSRVMAINTHREAD_H__

#include "mainThread.h"
#include "threadPool.h"
#include "chatSession.h"
#include "backSrvLogicThread.h"

namespace chat
{
struct BackSrvConfig;

class BackSrvMainThread : public parrot::MainThread<ChatSession, ChatSession>
{
  public:
    explicit BackSrvMainThread(const BackSrvConfig* cfg);

  protected:
    void beforeStart() override;
    void createUserThreads() override;
    void stopUserThreads() override;

  protected:
    parrot::ThreadPool<BackSrvLogicThread> _logicThreadPool;
};
}
#endif
