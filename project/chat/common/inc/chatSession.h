#ifndef __PROJECT_CHAT_FRONTSRV_INC_CHATSESSION_H__
#define __PROJECT_CHAT_FRONTSRV_INC_CHATSESSION_H__

#include <cstdint>
#include <string>

#include "jobHandler.h"

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

    void setFrontJobHdr(parrot::JobHandler *hdr);
    parrot::JobHandler * getFrontJobHdr() const;

    void setIpAddrPort(const std::string& ip, uint16_t port);
    const std::string &getIpAddr() const;
    uint16_t getPort() const;

    const std::string& getJsonStr() const;

    std::string toString() const;

  private:
    std::string _uniqueSessionId;
    std::string _uid;
    std::string _ipAddr;
    std::string _jsonStr;
    parrot::JobHandler *_frontJobHdr;
    uint16_t _port;
};
}
#endif
