#ifndef __COMPONENTS_SERVERGEAR_INC_FRONTTHREAD_H__
#define __COMPONENTS_SERVERGEAR_INC_FRONTTHREAD_H__

#include <unordered_map>
#include <memory>
#include <mutex>
#include <list>
#include <cstdint>

#include "poolThread.h"

namespace parrot
{
struct Config;
struct Session;
class EventNotifier;
class WsServerConn;
class Job;

template <typename Job> class FrontThread : public PoolThread
{
    using PacketHdrCb   = std::function<void(std::unique_ptr<WsPacket>&&)>;
    using DefaultPktHdr = std::function<void(std::unique_ptr<Job>)>;
    using OnPacketHdr = std::function<void(std::shared_ptr<const Session>&,
                                           std::unique_ptr<WsPacket>&&)>;
    using SessionPktPair =
        std::pair<std::shared_ptr<const Session>, std::unique_ptr<WsPacket>>;
    // <ThreadPtr, list<unique_ptr<WsPacket>>>
    using ThreadPktMap =
        std::unordered_map<void*, std::list<std::unique_ptr<WsPacket>>>;

    using ReqBindJob =
        ThreadJob<FrontThread*,
                  std::list<std::pair<std::shared_ptr<const Session>,
                                      std::unique_ptr<WsPacket>>>>;

    using RspBindJob = ThreadJob<std::shared_ptr<const Session>>;
    using RspBindHdr =
        std::function<void(std::shared_ptr<const Session>&)>;

    using UpdateSessionJob = ThreadJob<std::shared_ptr<const Session>>;
    using UpdateSessionHdr =
        std::function<void(std::shared_ptr<const Session &>)>;
    

    enum class Constants
    {
        kPktListSize = 100
    };

  public:
    FrontThread();

  public:
    void setConfig(const Config* cfg);
    void regisgerDefaultPktHandler(DefaultPktHdr&& defaultHdr);
    void registerOnPacketHandler(void* handler, OnPacketHdrCb);
    void addConn(std::list<std::shared_ptr<WsServerConn>>& connList);
    void addConn(std::shared_ptr<WsServerConn>&& conn);
    void addJob(std::unique_ptr<Job>&& job);
    void addJob(std::list<std::unique_ptr<Job>>& jobList);

  protected:
    void beforeStart() override;
    void run() override();

  private:
    std::list<SessionPktPair> _noRoutePktList;
    std::unordered_map<void*, std::list<SessionPktPair>> _pktMap;

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
    OnPacketHdr _onPktHdr;
    UpdateSessionHdr _updateSessionHdr;

    MtRandom _random;
    const Config* _config;
};
}

#endif
