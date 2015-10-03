#ifndef __BASE_SYS_INC_LISTENER_H__
#define __BASE_SYS_INC_LISTENER_H__

#include <cstdint>
#include <string>
#include <list>

#include "ipHelper.h"

namespace parrot
{
class IoEvent;
enum class eIoAction : uint8_t;

class Listener : public IoEvent
{
  public:
    Listener(uint16_t listenPort, const std::string& listenIp = "127.0.0.1");
    Listener(const Listener&) = delete;
    Listener& operator=(const Listener&) = delete;

  public:
    eIoAction handleIoEvent() override;
    void startListen();
    eCodes doAccept(int &cliFd, IPHelper &cliIP, uint16_t &cliPort);    

  private:
    IPHelper _listenIp;
    uint16_t _listenPort;
};
}

#endif
