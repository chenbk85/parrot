#ifndef __COMPONENT_SERVERGEAR_INC_RPCSERVERCONNMANAGER_H__
#define __COMPONENT_SERVERGEAR_INC_RPCSERVERCONNMANAGER_H__

#include <memory>
#include <list>
#include <cstdint>
#include <string>

#include "wsPacketHandler.h"
#include "wsPacket.h"
#include "wsServerConn.h"
#include "mtRandom.h"
#include "timeoutManager.h"
#include "connManager.h"
#include "rpcServerConn.h"
#include "rpcSession.h"

namespace parrot
{
class RpcServerThread;

class RpcServerConnManager
    : public ConnManager<RpcServerConn, WsServerConn<RpcSession>>
{
    using ConnMgr = ConnManager<RpcServerConn, WsServerConn<RpcSession>>;

  public:
    explicit RpcServerConnManager(RpcServerThread* thread);

  public:
    void registerConn(const std::string& sid, RpcServerConn* conn);
    std::unordered_map<std::string, RpcServerConn*>& getRegisteredConnMap();

  public:
    void removeConn(RpcServerConn* conn) override;    
    
  protected:
    void addConnToNotifier() override;
    void onTimeout(WsServerConn<RpcSession>*) override;

  private:
//    RpcServerThread* _thread;
    std::list<std::unique_ptr<RpcServerConn>> _localConnList;
    MtRandom _random;

    // <Remote sid, RpcServerConn>
    std::unordered_map<std::string, RpcServerConn*> _registeredConnMap;
};
}

#endif
