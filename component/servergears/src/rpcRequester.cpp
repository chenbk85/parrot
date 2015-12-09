#include <sstream>

#include "rpcRequester.h"
#include "jobHandler.h"

namespace parrot
{
RpcRequester::RpcRequester(JobHandler* hdr,
                           const std::string& uid,
                           const std::string& sid)
    : _rspHandler(hdr), _uid(uid), _sid(sid)
{
}

std::string RpcRequester::toString()
{
    std::ostringstream ostr;
    ostr << "Uid is " << _uid << ". Sid is " << _sid;
    return std::move(ostr.str());
}
}
