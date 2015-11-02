#ifndef __PROJECT_CHAT_FRONTSRV_INC_LOGICTHREAD_H__
#define __PROJECT_CHAT_FRONTSRV_INC_LOGICTHREAD_H__

#include <list>
#include <memory>
#include <mutex>

#include "poolThread.h"
#include "wsPacket.h"

namespace chat
{
class LogicThread : public parrot::PoolThread, public parrot::JobHandler
{
  public:
    LogicThread();

  public:
    void setConfig(const Config *cfg);
    
  public:
    void stop() override;
    void addJob(std::unique_ptr<Job>&& job) override;
    void addJob(std::list<std::unique_ptr<Job>>& jobList) override;

  protected:
    void beforeStart() override;
    void run() override;

  protected:
    void handleJob() override;

  protected:
    void handleReqBind(FrontThread* thread, std::list<SessionPktPair>& pktList);
    void handlePacket(std::list<SessionPktPair>& pktList);

  protected:
    ReqBindJobHdr _reqBindJobHdr;
    PacketJobHdr _packetJobHdr;
    std::mutex _jobListLock;
    std::list<std::unique_ptr<Job>> _jobList;
    std::unique_ptr<EventNotifier> _notifier;
    const Config * _config;
};
}
#endif
