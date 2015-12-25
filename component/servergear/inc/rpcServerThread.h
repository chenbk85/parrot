#ifndef __COMPONENT_SERVERGEAR_INC_RPCSERVERTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_RPCSERVERTHREAD_H__

namespace parrot
{
class RpcServerThread : public ThreadBase,
                        public TimeoutHandler<RpcServerConn<RpcSession>>,
                        public JobHandler,
                        public ConnHandler<RpcServerConn<RpcSession>>
{
    using PktList = std::list<std::tuple<std::shared_ptr<const RpcSession>,
                                         std::unique_ptr<Json>,
                                         std::unique_ptr<WsPacket>>>;

    using ConnMap =
        std::unordered_map<RpcServerConn*, std::unique_ptr<RpcServerConn>>;

  public:
    RpcServerThread();

  public:
    // ThreadBase
    void stop() override;

    void registerConn(const std::string& sid, RpcServerConn* conn);

    void addReqPacket(JobHandler* hdr,
                      std::shared_ptr<const RpcSession>,
                      std::unique_ptr<Json>&& cliSession,
                      std::unique_ptr<WsPacket>&& pkt);

  protected:
    void afterAddNewConn() override;

    void afterAddJob() override;

    // ThreadBase
    void run() override;

    // TimeoutHandler
    void onTimeout(RpcServerConn*) override;

    // JobHandler
    void handleJob() override;

  private:
    void handleRpcRsp(PktList& pktList);
    void dispatchPackets();
    void addConnToNotifier();
    void removeConn(RpcServerConn* conn);
    void updateTimeout(RpcServerConn* conn, std::time_t now);

  private:
    ConnMap _connMap;
    // <Remote sid, RpcServerConn>
    std::unordered_map<std::string, RpcSeverConn*> _registeredConnMap;

    std::unordered_map<JobHandler*, PktList> _reqMap;

    std::unique_ptr<EventNotifier> _notifier;
    std::unique_ptr<TimeoutManager<RpcServerConn>> _timeoutMgr;
    PacketJobHdr<RpcSession> _pktJobHdr;
};
}

#endif
