#ifndef __PROJECT_CHAT_BACKSRV_INC_BACKSRVMAINTHREAD_H__
#define __PROJECT_CHAT_BACKSRV_INC_BACKSRVMAINTHREAD_H__

#include <memory>

#include "mainThread.h"
#include "threadPool.h"
#include "chatSession.h"
#include "backSrvLogicThread.h"

namespace chat
{
struct BackSrvConfig;

class BackSrvMainThread : public parrot::MainThread<ChatSession, ChatSession>
{
  private:
    static std::unique_ptr<BackSrvMainThread> _instance;

  public:
    static void createInstance(const BackSrvConfig* cfg);
    static BackSrvMainThread* getInstance();

  private:
    explicit BackSrvMainThread(const BackSrvConfig* cfg);

  public:
    ~BackSrvMainThread() = default;

  public:
    parrot::ThreadPool<BackSrvLogicThread>& getLogicThreadPool();
    const BackSrvConfig * getConfig() const;    

  protected:
    void beforeStart() override;
    void createUserThreads() override;
    void stopUserThreads() override;

  protected:
    parrot::ThreadPool<BackSrvLogicThread> _logicThreadPool;
    const BackSrvConfig* _config;
};
}
#endif
