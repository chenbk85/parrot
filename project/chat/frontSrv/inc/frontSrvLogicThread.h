#ifndef __PROJECT_CHAT_FRONTSRV_INC_FRONTSRVLOGICTHREAD_H__
#define __PROJECT_CHAT_FRONTSRV_INC_FRONTSRVLOGICTHREAD_H__

#include <list>
#include <memory>
#include <mutex>

#include "poolThread.h"
#include "wsPacket.h"
#include "job.h"
#include "threadJob.h"
#include "jobHandler.h"

namespace chat
{
struct FrontSrvConfig;

class FrontSrvLogicThread : public parrot::PoolThread, public parrot::JobHandler
{
  public:
    FrontSrvLogicThread();

  public:
    void setConfig(const FrontSrvConfig* cfg);

  public:
    void stop() override;
    void addJob(std::unique_ptr<parrot::Job>&& job) override;
    void addJob(std::list<std::unique_ptr<parrot::Job>>& jobList) override;

  protected:
    void beforeStart() override;
    void run() override;

  protected:
    void handleJob() override;

  protected:
    void handlePacket(std::list<parrot::SessionPktPair>& pktList);

  protected:
    parrot::PacketJobHdr _packetJobHdr;
    std::mutex _jobListLock;
    std::list<std::unique_ptr<parrot::Job>> _jobList;
    std::unique_ptr<parrot::EventNotifier> _notifier;
    const FrontSrvConfig* _config;
};
}
#endif
