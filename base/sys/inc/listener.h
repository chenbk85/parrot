#ifndef __BASE_SYS_INC_LISTENER_H__
#define __BASE_SYS_INC_LISTENER_H__

#include <cstdint>
#include <string>
#include <list>
#include <vector>

#include "ipHelper.h"
#include "ioEvent.h"
#include "sysDefinitions.h"

namespace parrot
{

class Listener : public IoEvent
{
  public:
    Listener(uint16_t listenPort, const std::string& listenIp);
    Listener(const Listener&) = delete;
    Listener& operator=(const Listener&) = delete;

  public:
    virtual eIoAction handleIoEvent() = 0;
    void startListen();

  protected:
    eCodes doAccept(int& cliFd, IPHelper& cliIP, uint16_t& cliPort);

  protected:
    IPHelper _listenIp;
    uint16_t _listenPort;
};
}

#endif
