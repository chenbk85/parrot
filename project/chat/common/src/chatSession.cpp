#include <sstream>
#include "chatSession.h"
#include "macroFuncs.h"

namespace chat
{
ChatSession::ChatSession()
    : _mgrMap(),
      _uniqueSessionId(),
      _uid(),
      _ipAddr(),
      _jsonStr(),
      _frontJobMgr(nullptr),
      _port(0)
{
}

void ChatSession::setUid(const std::string& uid)
{
    _uid = uid;
}

const std::string& ChatSession::getUid() const
{
    return _uid;
}

void ChatSession::createUniqueSessionId(const std::string& srvId,
                                        const std::string& tid,
                                        uint32_t cid)
{
    _uniqueSessionId = std::to_string(cid) + "-" + tid + "-" + srvId;
}

const std::string& ChatSession::getUniqueSessionId() const
{
    return _uniqueSessionId;
}

void ChatSession::setFrontJobMgr(parrot::JobManager* mgr)
{
    _frontJobMgr = mgr;
}

parrot::JobManager* ChatSession::getFrontJobMgr() const
{
    PARROT_ASSERT(_frontJobMgr != nullptr);
    return _frontJobMgr;
}

void ChatSession::setIpAddrPort(const std::string& ip, uint16_t port)
{
    _ipAddr = ip;
    _port   = port;
}

const std::string& ChatSession::getIpAddr() const
{
    return _ipAddr;
}

uint16_t ChatSession::getPort() const
{
    return _port;
}

const std::string& ChatSession::getJsonStr() const
{
    return _jsonStr;
}

void ChatSession::setRouteJobMgr(uint64_t route, parrot::JobManager* mgr)
{
    _mgrMap[route] = mgr;
}

parrot::JobManager* ChatSession::getRouteJobMgr(uint64_t route) const
{
    auto it = _mgrMap.find(route);
    if (it == _mgrMap.end())
    {
        return nullptr;
    }

    return it->second;
}

std::string ChatSession::toString() const
{
    std::ostringstream ostr;
    ostr << "[Uid is " << _uid << ". IP is " << _ipAddr << "]";
    return ostr.str();
}
}
