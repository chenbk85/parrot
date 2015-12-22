#ifndef __COMPONENT_SERVERGEAR_INC_MAINTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_MAINTHREAD_H__

#include <vector>
#include <thread>
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
#include "wsConfig.h"
#include "config.h"
#include "threadBase.h"
#include "frontThread.h"
#include "daemon.h"
#include "epoll.h"
#include "logger.h"

namespace parrot
{
struct Config;

template <typename Sess>
class MainThread
{
  public:
    using FrontConnDispatcher =
        ConnDispatcher<WsServerConn<Sess>, WsConfig, ConnFactory, ConnHandler>;
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

template <typename Sess>
MainThread<Sess>::MainThread(const Config* cfg)
    : _connDispatcher(),
#if defined(__linux__)
      _notifier(new Epoll(100)),
#elif defined(__APPLE__)
      _notifier(new Kqueue(100)),
#elif defined(_WIN32)
//      _notifier(new SimpleEventNotifier()),
#endif
      _frontThreadPool(new FrontThreadPool(cfg->_frontThreadMaxConnCount)),
      _wsConfig(new WsConfig()),
      _config(cfg)
{
}

template <typename Sess>
void MainThread<Sess>::beforeStart()
{
    Daemon::setConfig(_config);

    // Make this process as daemon.
    //Daemon::daemonize();

    auto shutdownCb = std::bind(&MainThread<Sess>::onStop, this);
    Daemon::registerShutdownCb(shutdownCb);

    parrot::Logger* logger = parrot::Logger::instance();
    logger->setConfig(_config);
    logger->start();

    auto& sid = _config->_thisServer._serverId;
    LOG_INFO("*************************************************************");
    LOG_INFO("*   SERVICE " << sid << " STARTED.                           ");
    LOG_INFO("*************************************************************");

    _notifier->create();

    ConnFactory<WsServerConn<Sess>, WsConfig>::getInstance()->setConfig(
        _wsConfig.get());

    createListenerEvents();
    createThreads();
}

template <typename Sess>
void MainThread<Sess>::createThreads()
{
    Daemon::beforeCreateThreads();
    createSysThreads();
    createUserThreads();
    Daemon::afterCreateThreads();
}

template <typename Sess>
void MainThread<Sess>::createSysThreads()
{
    _frontThreadPool->setCount(_config->_frontThreadPoolSize);
    _frontThreadPool->create();

    std::vector<ConnHandler<WsServerConn<Sess>>*> vec;
    auto& threadVec = _frontThreadPool->getThreadPoolVec();
    for (auto& t : threadVec)
    {
        t->updateByConfig(_config);
        vec.push_back(t.get());
    }
    _connDispatcher->setConnHandler(std::move(vec));
}

template <typename Sess>
void MainThread<Sess>::createListenerEvents()
{
    auto& thisServer = _config->_thisServer;
    _connDispatcher.reset(
        new FrontConnDispatcher(thisServer._frontPort, thisServer._frontIp));

    _connDispatcher->setUrlInfo(thisServer._frontWsUrlInfo.get());
    _connDispatcher->setNextAction(eIoAction::Read);
    _connDispatcher->startListen();
    _notifier->addEvent(_connDispatcher.get());
}

template <typename Sess>
void MainThread<Sess>::start()
{
    beforeStart();
    _frontThreadPool->start();
    run();
    beforeTerminate();
}

template <typename Sess>
void MainThread<Sess>::run()
{
    int eventNum = 0;
    IoEvent* ev  = nullptr;

    while (!Daemon::isShutdown())
    {
        eventNum = _notifier->waitIoEvents(-1);
        for (int i = 0; i != eventNum; ++i)
        {
            ev = _notifier->getIoEvent(i);
            ev->handleIoEvent();
        }
    }
}

template <typename Sess>
void MainThread<Sess>::onStop()
{
    LOG_INFO("MainThread::onStop.");
    _notifier->stopWaiting();
}

template <typename Sess>
void MainThread<Sess>::stopSysThreads()
{
    _frontThreadPool->destroy();
}

template <typename Sess>
void MainThread<Sess>::stopThreads()
{
    stopUserThreads();
    stopSysThreads();
    LOG_INFO("MainThread::stopThreads: Done.");
}

template <typename Sess>
void MainThread<Sess>::beforeTerminate()
{
    LOG_INFO("MainThread::beforeTerminate.");
    stopThreads();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    parrot::Logger::instance()->stop();
    Daemon::beforeTerminate();
}
}

#endif
