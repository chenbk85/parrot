#ifndef __PROJECT_CHAT_FRONTSRV_INC_FRONTSRVMAINTHREAD_H__
#define __PROJECT_CHAT_FRONTSRV_INC_FRONTSRVMAINTHREAD_H__

#include "mainThread.h"
#include "threadPool.h"
#include "chatSession.h"
#include "logicThread.h"

namespace chat
{
struct FrontSrvConfig;

class FrontSrvMainThread : public parrot::MainThread<ChatSession, ChatSession>
{
  private:
    static std::unique_ptr<FrontSrvMainThread> _instance;

  public:
    static void createInstance(const FrontSrvConfig* cfg);
    static FrontSrvMainThread* getInstance();

  private:
    explicit FrontSrvMainThread(const FrontSrvConfig* cfg);

  public:
    ~FrontSrvMainThread() = default;

  public:
    parrot::ThreadPool<parrot::LogicThread>& getLogicThreadPool();
    const FrontSrvConfig * getConfig() const;

  protected:
    void beforeStart() override;
    void createUserThreads() override;
    void stopUserThreads() override;

  protected:
    const FrontSrvConfig* _config;
    parrot::ThreadPool<parrot::LogicThread> _logicThreadPool;
};
}
#endif
