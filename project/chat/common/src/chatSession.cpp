#include "chatSession.h"

namespace chat
{
ChatSession::ChatSession()
    : _uniqueSessionId(),
      _uid(),
      _ipAddr(),
      _jsonStr(),
      _frontJobHdr(nullptr),
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

void ChatSession::setFrontJobHdr(parrot::JobHandler* hdr)
{
    _frontJobHdr = hdr;
}

parrot::JobHandler* ChatSession::getFrontJobHdr() const
{
    return _frontJobHdr;
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

std::string ChatSession::toString() const
{
    return "";
}
}