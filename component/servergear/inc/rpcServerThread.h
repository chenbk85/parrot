#ifndef __COMPONENT_SERVERGEAR_INC_RPCSERVERTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_RPCSERVERTHREAD_H__

namespace parrot
{
class RpcServerThread : public ThreadBase,
                        public TimeoutHandler<RpcServerConn<RpcSession>>,
                        public JobHandler,
                        public ConnHandler<RpcServerConn<RpcSession>>
{
  public:
    RpcServerThread();

  public:
    // ThreadBase
    void stop() override;

    // JobHandler
    void addJob(std::unique_ptr<Job>&& job) override;
    void addJob(std::list<std::unique_ptr<Job>>& jobList) override;

    // ConnHandler<WsServerConn>
    void addConn(std::list<std::unique_ptr<WsServerConn<Sess>>>&) override;

  protected:
    // ThreadBase
    void run() override;

    // TimeoutHandler
    void onTimeout(WsServerConn<Sess>*) override;

    // JobHandler
    void handleJob() override;
};
}

#endif
