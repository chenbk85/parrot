#include "rpcServerThread.h"
#include "logger.h"

namespace parrot
{
RpcServerThread::RpcServerThread()
    : ThreadBase(),
      TimeoutHandler<RpcServerConn<RpcSession>>(),
      JobHandler(),
      ConnHandler<RpcServerConn<RpcSession>>(),
      _connMap(),
      _registeredConnMap(),
      _notifier(),
      _timeoutMgr(),
      _pktJobHdr()
{
}

void RpcServerThread::stop()
{
    ThreadBase::stop();
    _notifier->stopWaiting();
    ThreadBase::join();
    LOG_INFO("RpcServerThread::Stop: Done.");    
}

void RpcServerThread::registerConn(const std::string& sid, RpcServerConn* conn)
{
    PARROT_ASSERT(_connMap.find(conn) != _connMap.end());

    auto it = _registeredConnMap.find(sid);
    if (it != _registeredConnMap.end())
    {
        LOG_WARN("RpcServerThread::registerConn: Removing conn "
                 << it->second->getSession()->toString() << ".");
        removeConn(it->second);
    }

    _registeredConnMap[sid] = conn;
    LOG_INFO("RpcServerThread::addConn: Registered conn "
             << conn->getSession()->toString() << ".");
}

void RpcServerThread::afterAddJob()
{
    _notifier->stopWaiting();
}

void RpcServerThread::afterAddNewConn()
{
    _notifier->stopWaiting();
}

}
