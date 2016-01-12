#include "rpcSession.h"
#include "macroFuncs.h"

namespace parrot
{
RpcSession::RpcSession():
    _remoteSid(),
    _connPtr(nullptr)
{
}

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

void RpcSession::setConnPtr(void* conn)
{
    _connPtr = conn;
}

void* RpcSession::getConnPtr() const
{
    PARROT_ASSERT(_connPtr != nullptr);
    return _connPtr;
}

}
