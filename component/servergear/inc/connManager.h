#ifndef __COMPONENT_SERVERGEAR_INC_CONNMANAGER_H__
#define __COMPONENT_SERVERGEAR_INC_CONNMANAGER_H__

#include <list>
#include <memory>
#include <mutex>
#include <ctime>
#include <unordered_map>

#include "logger.h"
#include "eventNotifier.h"
#include "timeoutHandler.h"
#include "timeoutManager.h"

namespace parrot
{
template <typename Conn, typename TGuard>
class ConnManager : TimeoutHandler<TGuard>
{
  public:
    using ConnMap = std::unordered_map<void*, std::unique_ptr<Conn>>;

  public:
    ConnManager();
    virtual ~ConnManager() = default;

  public:
    void addConn(std::list<std::unique_ptr<Conn>>& connList);
    void createTimeManager(uint32_t timeout);
    ConnMap& getConnMap();

  protected:
    void setEventNotifier(EventNotifier* n);
    void checkTimeout(std::time_t now);

  protected:
    virtual void removeConn(Conn* conn);
    virtual void updateTimeout(Conn* conn, std::time_t now);

  protected:
    virtual void addConnToNotifier() = 0;
    virtual void onTimeout(TGuard*) = 0;

  protected:
    std::mutex _newConnListLock;
    std::list<std::unique_ptr<Conn>> _newConnList;
    std::unique_ptr<TimeoutManager<TGuard>> _timeoutMgr;
    std::unordered_map<void*, std::unique_ptr<Conn>> _connMap;
    EventNotifier* _baseNotifier;
};

template <typename Conn, typename TGuard>
ConnManager<Conn, TGuard>::ConnManager()
    : _newConnListLock(),
      _newConnList(),
      _timeoutMgr(),
      _connMap(),
      _baseNotifier(nullptr)
{
}

template <typename Conn, typename TGuard>
void ConnManager<Conn, TGuard>::setEventNotifier(EventNotifier* n)
{
    _baseNotifier = n;
}

template <typename Conn, typename TGuard>
std::unordered_map<void*, std::unique_ptr<Conn>>&
ConnManager<Conn, TGuard>::getConnMap()
{
    return _connMap;
}

template <typename Conn, typename TGuard>
void ConnManager<Conn, TGuard>::createTimeManager(uint32_t timeout)
{
    _timeoutMgr.reset(new TimeoutManager<TGuard>(this, timeout));
}

template <typename Conn, typename TGuard>
void ConnManager<Conn, TGuard>::checkTimeout(std::time_t now)
{
    _timeoutMgr->checkTimeout(now);
}

template <typename Conn, typename TGuard>
void ConnManager<Conn, TGuard>::addConn(
    std::list<std::unique_ptr<Conn>>& connList)
{
    _newConnListLock.lock();
    _newConnList.splice(_newConnList.end(), connList);
    _newConnListLock.unlock();

    _baseNotifier->stopWaiting();
}

template <typename Conn, typename TGuard>
void ConnManager<Conn, TGuard>::removeConn(Conn* conn)
{
    LOG_DEBUG("ConnManager::removeConn: Conn " << conn->getRemoteAddr()
                                               << " disconnected.");
    _timeoutMgr->remove(conn);
    _baseNotifier->delEvent(conn);
    _connMap.erase(conn->getSession()->getConnPtr());
}

template <typename Conn, typename TGuard>
void ConnManager<Conn, TGuard>::updateTimeout(Conn* conn, std::time_t now)
{
    _timeoutMgr->update(conn, now);
}
}

#endif
