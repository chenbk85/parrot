#include <sstream>

#include "rpcRequester.h"
#include "jobHandler.h"

namespace parrot
{
RpcRequester::RpcRequester(JobHandler* hdr, const std::string& sessionStr)
    : _rspHandler(hdr), _session(sessionStr)
{
}

std::string RpcRequester::toString()
{
    return _session;
}
}
