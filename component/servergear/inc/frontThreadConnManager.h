#ifndef __COMPONENT_SERVERGEAR_INC_FRONTTHREADCONNMANAGER_H__
#define __COMPONENT_SERVERGEAR_INC_FRONTTHREADCONNMANAGER_H__

#include "connManager.h"
#include "wsServerConn.h"
#include "wsPacketHandler.h"
#include "wsPacket.h"
#include "timeoutManager.h"
#include "frontThreadJobProcesser.h"

namespace parrot
{
template <typename Sess, template <typename> class Thread>
class FrontThreadConnManager
    : public ConnManager<WsServerConn<Sess>, WsServerConn<Sess>>,
      public WsPacketHandler<Sess, WsServerConn<Sess>>
{
    using ConnMgr = ConnManager<WsServerConn<Sess>, WsServerConn<Sess>>;

  public:
    explicit FrontThreadConnManager(Thread<Sess>*);

  protected:
    // WsPacketHandler
    void onPacket(WsServerConn<Sess>* conn,
                  std::unique_ptr<WsPacket>&&) override;
    void onClose(WsServerConn<Sess>* conn,
                 std::unique_ptr<WsPacket>&&) override;

    void addConnToNotifier() override;
    void onTimeout(WsServerConn<Sess>*) override;

  protected:
    void setJobProcesser(FrontThreadJobProcesser<Sess>* jp);

  private:
    Thread<Sess>* _thread;
    std::list<std::unique_ptr<WsServerConn<Sess>>> _localConnList;
    uint64_t _connUniqueIdx;
    MtRandom _random;
    FrontThreadJobProcesser<Sess>* _baseJobProcesser;
};

template <typename Sess, template <typename> class Thread>
FrontThreadConnManager<Sess, Thread>::FrontThreadConnManager(Thread<Sess>* t)
    : ConnManager<WsServerConn<Sess>, WsServerConn<Sess>>(),
      WsPacketHandler<Sess, WsServerConn<Sess>>(),
      _thread(t),
      _localConnList(),
      _connUniqueIdx(0),
      _random(),
      _baseJobProcesser(nullptr)
{
}

template <typename Sess, template <typename> class Thread>
void FrontThreadConnManager<Sess, Thread>::setJobProcesser(
    FrontThreadJobProcesser<Sess>* jp)
{
    _baseJobProcesser = jp;
}

template <typename Sess, template <typename> class Thread>
void FrontThreadConnManager<Sess, Thread>::addConnToNotifier()
{
    ConnMgr::_newConnListLock.lock();
    _localConnList = std::move(ConnMgr::_newConnList);
    ConnMgr::_newConnListLock.unlock();

    auto now = std::time(nullptr);

    for (auto& c : _localConnList)
    {
        std::shared_ptr<Sess> sess(new Sess());
        sess->createUniqueSessionId(_thread->getServerSid(),
                                    _thread->getThreadIdStr(),
                                    _connUniqueIdx++);
        sess->setIpAddrPort(c->getRemoteAddr(), c->getRemotePort());
        sess->setFrontJobMgr(_thread);
        sess->setConnPtr(c.get());

        c->setSession(std::move(sess));
        c->setNextAction(c->getDefaultAction());
        c->setPacketHandler(this);
        c->setRandom(&_random);

        ConnMgr::_timeoutMgr->add(c.get(), now);
        ConnMgr::_baseNotifier->addEvent(c.get());

        LOG_DEBUG("FrontThreadConnManager::addConnToNotifier: Add connection "
                  << c->getRemoteAddr() << ".");
        ConnMgr::_connMap[c->getSession()->getConnPtr()] = std::move(c);
    }

    _localConnList.clear();
}

template <typename Sess, template <typename> class Thread>
void FrontThreadConnManager<Sess, Thread>::onTimeout(WsServerConn<Sess>* conn)
{
    std::unique_ptr<WsPacket> pkt(new WsPacket());
    pkt->setOpCode(eOpCode::Close);
    pkt->setClose(eCodes::WS_NormalClosure);
    onClose(conn, std::move(pkt));
}

template <typename Sess, template <typename> class Thread>
void FrontThreadConnManager<Sess, Thread>::onPacket(
    WsServerConn<Sess>* conn, std::unique_ptr<WsPacket>&& pkt)
{
    // A copy of shard_ptr. Need to move next.
    auto session = conn->getSession();
    auto route   = pkt->getRoute();
    auto mgr = session->getRouteJobMgr(route);
    if (!mgr)
    {
        mgr = Scheduler<Sess>::getInstance()->getJobManager(route, session);
        if (!mgr)
        {
            LOG_WARN(
                "FrontThreadConnManager::onPacket: Failed to find job manager "
                "for session "
                << session->toString() << ". Route is " << route << ".");
            return;
        }
        session->setRouteJobMgr(route, mgr);
    }

    if (!mgr)
    {
        LOG_WARN("FrontThreadConnManager::onPacket: Failed to find job manager "
                 "for session "
                 << session->toString() << ". Route is " << pkt->getRoute()
                 << ".");
        return;
    }

    _baseJobProcesser->createPacketJobs(
        mgr, PacketJobParam<Sess>(std::move(session), std::move(pkt)));
}

template <typename Sess, template <typename> class Thread>
void FrontThreadConnManager<Sess, Thread>::onClose(
    WsServerConn<Sess>* conn, std::unique_ptr<WsPacket>&& pkt)
{
    auto session = conn->getSession(); // A copy of shared_ptr.
    auto mgr = Scheduler<Sess>::getInstance()->getOnCloseJobManager(session);
    if (!mgr)
    {
        LOG_WARN(
            "FrontThreadConnManager::onClose: Failed to find mgr for session "
            << session->toString() << ".");
        ConnMgr::removeConn(conn);
        return;
    }

    _baseJobProcesser->createPacketJobs(
        mgr, PacketJobParam<Sess>(std::move(session), std::move(pkt)));

    ConnMgr::removeConn(conn);
    LOG_INFO("FrontThreadConnManager::onClose: Err is "
             << (uint32_t)(pkt->getCloseCode()) << ". Sess is "
             << session->toString() << ".");
}
}

#endif
