#ifndef __PROJECT_CHAT_FRONTSRV_INC_CHATSESSION_H__
#define __PROJECT_CHAT_FRONTSRV_INC_CHATSESSION_H__

#include <cstdint>
#include <string>
#include <unordered_map>

#include "jobManager.h"

namespace chat
{
class ChatSession
{
  public:
    ChatSession();

  public:
    void setUid(const std::string& uid);
    const std::string& getUid() const;

    // cid + '-' + tid + '-' + srvId
    void createUniqueSessionId(const std::string& srvId,
                               const std::string& tid,
                               uint32_t cid);
    const std::string& getUniqueSessionId() const;

    void setFrontJobMgr(parrot::JobManager* hdr);
    parrot::JobManager* getFrontJobMgr() const;

    void setConnPtr(void*);
    void* getConnPtr() const;

    void setIpAddrPort(const std::string& ip, uint16_t port);
    const std::string& getIpAddr() const;
    uint16_t getPort() const;

    const std::string& getJsonStr() const;

    std::string toString() const;

    void setRouteJobMgr(uint64_t route, parrot::JobManager*);
    parrot::JobManager* getRouteJobMgr(uint64_t route) const;

  private:
    std::unordered_map<uint64_t, parrot::JobManager*> _mgrMap;
    std::string _uniqueSessionId;
    std::string _uid;
    std::string _ipAddr;
    std::string _jsonStr;
    parrot::JobManager* _frontJobMgr;
    void* _connPtr;
    uint16_t _port;
};
}
#endif
