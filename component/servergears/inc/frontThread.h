#ifndef __COMPONENT_SERVERGEAR_INC_FRONTTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_FRONTTHREAD_H__

#include <unordered_map>
#include <memory>
#include <mutex>
#include <list>
#include <cstdint>
#include <ctime>

#include "mtRandom.h"
#include "poolThread.h"
#include "jobHandler.h"
#include "threadJob.h"
#include "timeoutHandler.h"
#include "timeoutManager.h"
#include "session.h"
#include "wsServerConn.h"
#include "eventNotifier.h"
#include "wsPacket.h"

namespace parrot
{
struct Config;

class FrontThread : public PoolThread,
                    public TimeoutHandler<WsServerConn>,
                    public JobHandler,
                    public ConnAcceptor<WsServerConn>,
                    public WsPacketHandler<Session, WsServerConn>
{
    // <ThreadPtr, list<unique_ptr<WsPacket>>>
    using ThreadJobMap =
        std::unordered_map<void*, std::list<SessionPktPair>>;

    enum class Constants
    {
        kPktListSize = 100
    };

  public:
    FrontThread();
    virtual ~FrontThread() = default;

  public:
    void updateByConfig(const Config* cfg);
    void setDefaultJobHdr(std::vector<JobHandler*>& hdr);

  public:
    // ThreadBase
    void stop() override;    
  protected:
    // ThreadBase
    void beforeStart() override;
    void run() override;


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
    void onClose(WsServerConn* conn, std::unique_ptr<WsPacket> &&) override;

  protected:
    void handleRspBind(std::list<std::shared_ptr<const Session>>& sl);
    void handleUpdateSession(std::shared_ptr<const Session>& ps);
    void dispatchPackets();
    void addConnToNotifier();
    void removeConn(WsServerConn* conn);
    void updateTimeout(WsServerConn* conn, std::time_t now);

  private:
    std::list<SessionPktPair> _noRoutePktList;
    std::unordered_map<void*, std::list<SessionPktPair>> _pktMap;

    std::unordered_map<void*, JobHandler*> _jobHandlerMap;
    ThreadJobMap _threadJobMap;

    std::unordered_map<uint64_t, std::shared_ptr<WsServerConn>> _connMap;

    std::unique_ptr<EventNotifier> _notifier;

    std::vector<JobHandler*> _defaultJobHdrVec;
    uint32_t _lastDefaultJobIdx;
    RspBindJobHdr _rspBindHdr;
    UpdateSessionJobHdr _updateSessionHdr;

    MtRandom _random;
    std::unique_ptr<TimeoutManager<WsServerConn>> _timeoutMgr;
    const Config* _config;
};
}

#endif
