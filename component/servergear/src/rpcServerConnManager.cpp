#include <ctime>

#include "logger.h"
#include "rpcServerConnManager.h"
#include "rpcServerThread.h"

namespace parrot
{
RpcServerConnManager::RpcServerConnManager(RpcServerThread* /*thread*/)
    : /* _thread(thread), */ _localConnList(), _random(), _registeredConnMap()
{
}

void RpcServerConnManager::addConnToNotifier()
{
    ConnMgr::_newConnListLock.lock();
    _localConnList = std::move(ConnMgr::_newConnList);
    ConnMgr::_newConnListLock.unlock();

    std::time_t now = std::time(nullptr);

    for (auto& c : _localConnList)
    {
        std::shared_ptr<RpcSession> sess(new RpcSession());
        c->setSession(std::move(sess));
        c->setNextAction(c->getDefaultAction());
        c->setRandom(&_random);

        ConnMgr::_timeoutMgr->add(c.get(), now);
        ConnMgr::_baseNotifier->addEvent(c.get());

        LOG_DEBUG("RpcServerConnManager::addConnToNotifier: Add connection "
                  << c->getRemoteAddr() << ".");
        ConnMgr::_connMap[c.get()] = std::move(c);
    }

    _localConnList.clear();
}

void RpcServerConnManager::onTimeout(WsServerConn<RpcSession>* conn)
{
    LOG_WARN("RpcServerConnManager::onTimeout: Remote sid is "
             << conn->getSession()->toString() << ".");
    ConnMgr::removeConn(static_cast<RpcServerConn*>(conn->getDerivedPtr()));
}

void RpcServerConnManager::registerConn(const std::string& sid,
                                        RpcServerConn* conn)
{
    PARROT_ASSERT(ConnMgr::_connMap.find(conn) != ConnMgr::_connMap.end());

    auto it = _registeredConnMap.find(sid);
    if (it != _registeredConnMap.end())
    {
        // Found same server id!!! Kick this connection.
        LOG_WARN("RpcServerConnManager::registerConn: Removing conn "
                 << it->second->getSession()->toString() << ".");
        ConnMgr::removeConn(it->second);
    }

    // Add the new connection to the connection map.
    _registeredConnMap[sid] = conn;
    LOG_DEBUG("RpcServerConnManager::Registerconn: Registered conn "
              << conn->getSession()->toString() << ".");
}

std::unordered_map<std::string, RpcServerConn*>&
RpcServerConnManager::getRegisteredConnMap()
{
    return _registeredConnMap;
}

void RpcServerConnManager::removeConn(RpcServerConn* conn)
{
    ConnMgr::removeConn(conn);
    _registeredConnMap.erase(conn->getSession()->getRemoteSid());
}
}
