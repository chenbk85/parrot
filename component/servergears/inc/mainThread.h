#ifndef __COMPONENT_SERVERGEAR_INC_MAINTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_MAINTHREAD_H__

#include <string>
#include <memory>
#include <cstdint>

#include "threadPool.h"
#include "eventNotifier.h"
#include "listener.h"
#include "connFactory.h"

namespace parrot
{
struct Config;
class WsServerConn;

class MainThread
{
  public:
    using FrontConnDispatcher = std::unique_ptr<
        ConnDispatcher<WsServerConn, ConnFactory, ConnAcceptor>>;
    using FrontThreadPool = std::unique_ptr<ThreadPool<FrontThread>>;

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
    void setFrontConnAcceptor(std::vector<ConnAcceptor<WsServerConn>*>&& );
    void setFrontConnAcceptor(std::vector<ConnAcceptor<WsServerConn>*>& );

  public:
    virtual void start();

  protected:
    virtual void beforeStart();
    virtual void createUserThreads() = 0;
    virtual void run();
    virtual void beforeTerminate();
    virtual void createListenerEvent();

  protected:
    void createSysThreads();
    void createThreads();
    void daemonize();

  protected:
    FrontConnDispatcher _connDispatcher;
    std::unique_ptr<EventNotifier> _notifier;
    FrontThreadPool _frontThreadPool;
    const Config* _config;
};
}

#endif
