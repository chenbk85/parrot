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
class Job;

template<typename Job>
class FrontThread : public PoolThread
{
    using PacketHdrCb = std::function<void(std::unique_ptr<WsPacket>&&)>;
    using DefaultPktHdr = std::function<void(std::unique_ptr<Job>)>;
        

    // <ThreadPtr, list<unique_ptr<WsPacket>>>
    using ThreadPktMap =
        std::unordered_map<void *, std::list<std::unique_ptr<WsPacket>>>;

    using ReqBindJob = ThreadJob<FrontThread *,
        std::list<std::pair<uint64_t, std::unique_ptr<WsPacket>>>>;

    using RspBindJob = ThreadJob<uint64_t, std::shared_ptr<Session>>;
    using RspBindHdr =
        std::function<void(uint64_t, std::shared_ptr<Session> &ps)>;
        
    enum class Constants
    {
        kPktListSize = 100
    };

  public:
    FrontThread();

  public:
    void setConfig(const Config* cfg);
    void regisgerDefaultPktHandler(DefaultPktHdr&& defaultHdr);
    void registerOnPacketHandler(void *handler, OnPacketHdrCb);
    void addConn(std::list<std::shared_ptr<WsServerConn>>& connList);
    void addConn(std::shared_ptr<WsServerConn>&& conn);
    void addJob(std::unique_ptr<Job> &&job);
    void addJob(std::list<std::unique_ptr<Job>> &jobList)

  protected:
    void beforeStart() override;
    void run() override();

  private:

    std::list<std::pair<uint64_t, std::unique_ptr<WsPacket>>> _noRoutePktList;
    
    std::unordered_map<void*, AddPktFunc>> _pktHandlerFuncMap;
    ThreadPktMap _threadPktMap;
    
    std::mutex _newConnListLock;
    std::list<std::shared_ptr<WsServerConn>> _newConnList;
    std::unordered_map<uint64_t, std::shared_ptr<WsServerConn>> _connMap;

    std::unique_ptr<EventNotifier> _notifier;

    std::mutex _jobListLock;
    std::list<std::unique_ptr<Job>> _jobList;

    DefaultPktHdr _defaultPktHdr;
    RspBindHdr _rspBindHdr;
    
    const Config* _config;
};
}

#endif
