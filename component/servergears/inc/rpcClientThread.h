#ifndef __COMPONENT_SERVERGEAR_INC_RPCCLIENTTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_RPCCLIENTTHREAD_H__

#include "threadBase.h"
#include "mtRandom.h"
#include "jobHandler.h"
#include "threadJob.h"
#include "timeoutHandler.h"
#include "timeoutManager.h"
#include "wsClientConn.h"
#include "eventNotifier.h"
#include "wsPacket.h"
#include "connHandler.h"

namespace parrot
{
class RpcClientThread : public ThreadBase,
                        public TimeoutHandler<WsClientConn>,
                        public JobHandler,
                        public ConnHandler<WsClientConn>,
                        public WsPacketHandler<Session, WsClientConn>
{
};
}

#endif
