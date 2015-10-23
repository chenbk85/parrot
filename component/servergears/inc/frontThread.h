#ifndef __COMPONENT_SERVERGEAR_INC_FRONTTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_FRONTTHREAD_H__

#include <unordered_map>
#include <memory>
#include <mutex>
#include <list>
#include <cstdint>
#include <ctime>

#include "poolThread.h"
#include "jobHandler.h"
#include "threadJob.h"
#include "timeoutHandler.h"

namespace parrot
{
struct Config;
struct Session;
class EventNotifier;
class WsServerConn;

class FrontThread : public PoolThread,
                    public TimeoutHandler<WsServerConn>,
                    public JobHandler,
                    public WsPacketHandler<Session, WsServerConn>
{
    using SessionPktPair =
        std::pair<std::shared_ptr<const Session>, std::unique_ptr<WsPacket>>;
    // <ThreadPtr, list<unique_ptr<WsPacket>>>
    using ThreadJobMap =
        std::unordered_map<void*, std::list<std::unique_ptr<Job>>>;

    enum class Constants
    {
        kPktListSize = 100
    };

  public:
    FrontThread();

  public:
    void updateByConfig(const Config* cfg);
    void setDefaultJobHdr(std::vector<JobHandler*>& hdr);
    void addConn(std::list<std::shared_ptr<WsServerConn>>& connList);

  protected:
    // ThreadBase
    void beforeStart() override;
    void run() override;
    void stop() override;

  protected:
    // TimeoutHandler
    void onTimeout(WsServerConn*) override;

  protected:
    // JobHandler
    void handleJob() override;

  public:
    // WsPacketHandler
    void onPacket(std::shared_ptr<const Session>&&,
                  std::unique_ptr<WsPacket>&&) override;
    void onClose(WsServerConn* conn, eCodes err) override;

  protected:
    void handleRspBind(std::list<std::shared_ptr<const Session>>& sl);
    void handleUpdateSession(std::shared_ptr<const Session>& ps);
    void onPacket(std::shared_ptr<const Session>&& session,
                  std::unique_ptr<WsPacket>&& pkt);
    void dispatchPackets();
    void addConnToNotifier();
    void removeConn(WsServerConn* conn);
    void updateTimeout(WsServerConn* conn, std::time_t now);

  private:
    std::list<SessionPktPair> _noRoutePktList;
    std::unordered_map<void*, std::list<std::unique_ptr<Job>>> _pktMap;

    std::unordered_map<void*, AddPktFunc>> _pktHandlerFuncMap;
    ThreadJobMap _threadJobMap;

    std::mutex _newConnListLock;
    std::list<std::shared_ptr<WsServerConn>> _newConnList;
    std::unordered_map<uint64_t, std::shared_ptr<WsServerConn>> _connMap;

    std::unique_ptr<EventNotifier> _notifier;

    std::vector<JobHandler*> _defaultJobHdrVec;
    uint32_t _lastDefaultJobIdx;
    RspBindJobHdr _rspBindHdr;
    UpdateSessionJobHdr _updateSessionHdr;

    MtRandom _random;
    std::unique_ptr<TimeoutManager<WsServerConn>> _timeougMgr;
    const Config* _config;
};
}

#endif
