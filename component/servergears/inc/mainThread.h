#ifndef __COMPONENT_SERVERGEAR_INC_MAINTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_MAINTHREAD_H__

#include <string>
#include <memory>
#include <cstdint>

#include "threadPool.h"
#include "eventNotifier.h"
#include "listener.h"
#include "connFactory.h"
#include "connHandler.h"
#include "connDispatcher.h"
#include "frontThread.h"

namespace parrot
{
struct Config;
class WsServerConn;

class MainThread
{
  public:
    using FrontConnDispatcher =
        ConnDispatcher<WsServerConn, ConnFactory, ConnHandler>;
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
    void setFrontConnHandler(std::vector<ConnHandler<WsServerConn>*>&&);
    void setFrontConnHandler(std::vector<ConnHandler<WsServerConn>*>&);

  public:
    virtual void start();

  protected:
    virtual void beforeStart();
    virtual void createUserThreads() = 0;
    virtual void run();
    virtual void beforeTerminate();
    virtual void createListenerEvents();

  protected:
    void createSysThreads();
    void createThreads();
    void daemonize();

  protected:
    std::unique_ptr<FrontConnDispatcher> _connDispatcher;
    std::unique_ptr<EventNotifier> _notifier;
    std::unique_ptr<FrontThreadPool> _frontThreadPool;
    const Config* _config;
};
}

#endif
