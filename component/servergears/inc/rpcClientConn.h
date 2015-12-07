#ifndef __COMPONENT_SERVERGEAR_INC_RPCCLIENTCONN_H__
#define __COMPONENT_SERVERGEAR_INC_RPCCLIENTCONN_H__

#include <string>
#include "wsClientConn.h"

namespace parrot
{
struct WsConfig;

class RpcClientConn : public WsClientConn
{
  public:
    RpcClientConn(const std::string& wsUrl, const WsConfig& cfg);

  private:
    uint64_t _reqId;
};
}

#endif
