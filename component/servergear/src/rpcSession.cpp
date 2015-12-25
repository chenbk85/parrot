#include "rpcSession.h"

namespace parrot
{
const std::string& RpcSession::getRemoteSid() const
{
    return _remoteSid;
}

void RpcSession::setRemoteSid(const std::string& sid)
{
    _remoteSid = sid;
}

std::string RpcSession::toString() const
{
    return _remoteSid;
}
}
