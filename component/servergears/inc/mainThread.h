#ifndef __COMPONENT_SERVERGEAR_INC_MAINTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_MAINTHREAD_H__

#include <string>
#include <memory>
#include <cstdint>
#include <unordered_set>

#include "threadPool.h"
#include "eventNotifier.h"
#include "listener.h"
#include "connFactory.h"
#include "connHandler.h"
#include "connDispatcher.h"
#include "frontThread.h"
#include "wsServerConn.h"
#include "session.h"
#include "wsConfig.h"

namespace parrot
{
struct Config;

class MainThread
{
  public:
    using FrontConnDispatcher =
        ConnDispatcher<WsServerConn<Session>, WsConfig, ConnFactory, ConnHandler>;
    using FrontThreadPool = ThreadPool<FrontThread>;

  public:
    explicit MainThread(const Config* cfg);
    virtual ~MainThread() = default;

  public:
    // onStop
    //
    // onStop is the callback which will be called when the
    // process received the shutdown signal.
    void onStop();

  public:
    void setFrontThreadDefaultJobHandler(std::vector<JobHandler*>& hdrs);
    void setFrontThreadJobHandler(std::unordered_set<JobHandler*>& hdrs);

  public:
    virtual void start();

  protected:
    virtual void beforeStart();
    virtual void createUserThreads() = 0;
    virtual void run();
    virtual void stopUserThreads() = 0;
    virtual void beforeTerminate();
    virtual void createListenerEvents();

  protected:
    void createSysThreads();
    void createThreads();
    void stopSysThreads();
    void stopThreads();

  protected:
    std::unique_ptr<FrontConnDispatcher> _connDispatcher;
    std::unique_ptr<EventNotifier> _notifier;
    std::unique_ptr<FrontThreadPool> _frontThreadPool;
    std::unique_ptr<WsConfig> _wsConfig;
    const Config* _config;
};
}

#endif
