#ifndef __BASE_SYS_INC_TRANSPORT_H__
#define __BASE_SYS_INC_TRANSPORT_H__

#include <cstdint>

namespace parrot {
class Transport {
  public:
    virtual Transport() {
    }
    Transport(const Transport &) = delete;
    Transport &operator=(const Transport &) = delete;

  public:
    virtual void getRecvBuff(char *&buff, uint32_t &len) = 0;
    virtual void getSendBuff(char *&buff, uint32_t &len) = 0;
    virtual void setRecvedLen(uint32_t len) = 0;
    virtual void setSentLen(uint32_t len) = 0;
};
}

#endif
