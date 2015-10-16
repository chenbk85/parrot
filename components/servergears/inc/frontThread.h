#ifndef __COMPONENT_SERVERGEAR_INC_FRONTTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_FRONTTHREAD_H__

#include <unordered_map>
#include <memory>
#include <mutex>
#include <list>
#include <cstdint>

#include "poolThread.h"

namespace parrot
{
struct Config;
class EventNotifier;
class WsServerConn;

template<typename Job>
class FrontThread : public PoolThread
{
    using AddPktFunc =
        std::function<void(std::list<std::unique_ptr<WsPacket>>)>;

    // <ThreadPtr, list<unique_ptr<WsPacket>>>
    using ThreadPktMap =
        std::unordered_map<void *, std::list<std::unique_ptr<WsPacket>>>;

    enum class Constants
    {
        PKT_LIST_SIZE = 100
    };

  public:
    FrontThread();

  public:
    void setConfig(const Config* cfg);
    void registerAddPktCb(void *threadPtr, AddPktFunc && func);
    void addConn(std::list<std::shared_ptr<WsServerConn>>& connList);
    void addConn(std::shared_ptr<WsServerConn>&& conn);






    

  protected:
    void beforeStart() override;
    void run() override();

  private:
    std::unordered_map<void*, AddPktFunc>> _pktHandlerFuncMap;
    ThreadPktMap _threadPktMap;
    
    std::mutex _newConnListLock;
    std::list<std::shared_ptr<WsServerConn>> _newConnList;

    std::unordered_map<uint64_t, std::shared_ptr<WsServerConn>> _connMap;

    std::unique_ptr<EventNotifier> _notifier;

    const Config* _config;
};
}

#endif
