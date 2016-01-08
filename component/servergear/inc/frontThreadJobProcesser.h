#ifndef __COMPONENT_SERVERGEAR_INC_FRONTTHREADJOBPROCESSER_H__
#define __COMPONENT_SERVERGEAR_INC_FRONTTHREADJOBPROCESSER_H__

#include "jobProcesser.h"
#include "threadJob.h"
#include "frontThread.h"

namespace parrot
{
class JobHandler;
template <typename Sess> class FrontThread;

template <typename Sess> class FrontThreadJobProcesser : public JobProcesser
{
  public:
    explicit FrontThreadJobProcesser(FrontThread<Sess>*);

  public:
    void createPacketJobs(JobHandler*, PacketJobParam<Sess> &&jobParam);

  public:
    void processJobs() override;

  protected:
    void loadJobs() override;

  protected:
    void processUpdateSession(std::list<UpdateSessionJobParam<Sess>>&);
    void processPacket(std::list<PacketJobParam<Sess>>& pktList);

  protected:
    PacketJobFactory<Sess> _pktJobFactory;
    UpdateSessionAckJobFactory<Sess> _updateSessAckJobFactory;

    UpdateSessionJobHdr<Sess> _updateSessionHdr;
    PacketJobHdr<Sess> _pktJobHdr;

    FrontThread<Sess>* _frontThread;
};

template <typename Sess>
FrontThreadJobProcesser<Sess>::FrontThreadJobProcesser(
    FrontThread<Sess>* thread)
    : JobProcesser(),
      _pktJobFactory(),
      _updateSessAckJobFactory(),
      _updateSessionHdr(),
      _pktJobHdr(),
      _frontThread(thread)
{
    using namespace std::placeholders;

    _updateSessionHdr =
        std::bind(&FrontThreadJobProcesser::processUpdateSession, this, _1);
    _pktJobHdr = std::bind(&FrontThreadJobProcesser::processPacket, this, _1);
}

template <typename Sess>
void FrontThreadJobProcesser<Sess>::createPacketJobs(
    JobHandler* hdr, PacketJobParam<Sess>&& jobParam)
{
    _pktJobFactory.add(hdr, std::move(jobParam));
}

template <typename Sess> void FrontThreadJobProcesser<Sess>::processJobs()
{
    for (auto& j : _jobList)
    {
        switch (j->getJobType())
        {
            case JOB_UPDATE_SESSION:
            {
                std::unique_ptr<UpdateSessionJob<Sess>> tj(
                    static_cast<UpdateSessionJob<Sess>*>(
                        (j.release())->getDerivedPtr()));
                tj->call(_updateSessionHdr);
            }
            break;

            case JOB_PACKET:
            {
                std::unique_ptr<PacketJob<Sess>> tj(
                    static_cast<PacketJob<Sess>*>(
                        (j.release())->getDerivedPtr()));
                tj->call(_pktJobHdr);
            }
            break;

            case JOB_KICK:
            {
            }
            break;

            default:
            {
                PARROT_ASSERT(false);
            }
            break;
        }
    }

    //
    loadJobs();

    //
    dispatchJobs();
}

template <typename Sess> void FrontThreadJobProcesser<Sess>::loadJobs()
{
    _pktJobFactory.loadJobs(_hdrJobListMap);
    _updateSessAckJobFactory.loadJobs(_hdrJobListMap);
}

template <typename Sess>
void FrontThreadJobProcesser<Sess>::processUpdateSession(
    std::list<UpdateSessionJobParam<Sess>>& newSessList)
{
    auto& connMap = _frontThread->_connMap;
    auto it = connMap.end();
    for (auto& kv : newSessList)
    {
        it = connMap.find((kv.second)->getUniqueSessionId());
        if (it == connMap.end())
        {
            // If the session is not here, the session must be disconnected,
            // and uplayer will receive session removed event. So we will not
            // notify uplayer that updating session is failed.
            LOG_WARN("FrontThreadJobProcesser::handleUpdateSession: Failed to "
                     "bind conn key "
                     << (kv.second)->getUniqueSessionId() << ". Sess is "
                     << (kv.second)->toString() << ".");
            return;
        }

        it->second->updateSession(kv.second);
        _updateSessAckJobFactory.add(kv.first, std::move(kv.second));
    }
}

template <typename Sess>
void FrontThreadJobProcesser<Sess>::processPacket(
    std::list<PacketJobParam<Sess>>& pktList)
{
    typename FrontThread<Sess>::ConnMap::iterator it;
    auto& connMap = _frontThread->_connMap;

    for (auto& s : pktList)
    {
        it = connMap.find((s.first)->getUniqueSessionId());
        if (it == connMap.end())
        {
            LOG_DEBUG("FrontThreadJobProcesser::processPacket: Failed to find "
                      "session "
                      << (s.first)->toString() << ".");
            continue;
        }

        LOG_DEBUG(
            "FrontThreadJobProcesser::processPacket: Send packet to session "
            << (s.first)->toString() << ".");
        it->second->sendPacket(s.second);

        if (it->second->canSwitchToSend())
        {
            it->second->setNextAction(eIoAction::Write);
            _frontThread->_notifier->updateEventAction(it->second.get());
        }
    }
}
}

#endif
