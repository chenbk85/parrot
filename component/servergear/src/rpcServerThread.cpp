#include "rpcServerThread.h"

namespace parrot
{
RpcServerThread::RpcServerThread()
    : ThreadBase(),
      TimeoutHandler<RpcServerConn<RpcSession>>(),
      JobHandler(),
      ConnHandler<RpcServerConn<RpcSession>>()
{
}

void RpcServerThread::stop()
{
}

void RpcServerThread::addJob(std::unique_ptr<Job>&& job)
{
}

void RpcServerThread::addJob(std::list<std::unique_ptr<Job>>& jobList)
{
}

void RpcServerThread::addConn(std::list<std::unique_ptr<WsServerConn<Sess>>>&)
{
}
}
