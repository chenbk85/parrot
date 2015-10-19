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
class ThreadJob;

template<typename Job>
class FrontThread : public PoolThread
{
    using AddPktFunc =
        std::function<void(std::list<std::unique_ptr<WsPacket>>)>;

    using PacketHdrCb =
        std::function<void(std::unique_ptr<WsPacket>&&)>;

    // <ThreadPtr, list<unique_ptr<WsPacket>>>
    using ThreadPktMap =
        std::unordered_map<void *, std::list<std::unique_ptr<WsPacket>>>;

    enum class Constants
    {
        kPktListSize = 100
    };

  public:
    FrontThread();

  public:
    void setConfig(const Config* cfg);
    void registerOnPacketHandler(void *handler, OnPacketHdrCb);
    void registerAddPktCb(void *threadPtr, AddPktFunc && func);
    void addConn(std::list<std::shared_ptr<WsServerConn>>& connList);
    void addConn(std::shared_ptr<WsServerConn>&& conn);
    void addJob(std::unique_ptr<ThreadJob> &&job);
    void addJob(std::list<std::unique_ptr<ThreadJob>> &jobList)

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

    std::mutex _jobListLock;
    std::list<std::unique_ptr<ThreadJob>> _jobList;
    
    const Config* _config;
};
}

#endif
