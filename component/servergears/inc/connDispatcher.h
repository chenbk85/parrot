#ifndef __COMPONENT_SERVERGEAR_INC_CONNDISPATCHER_H__
#define __COMPONENT_SERVERGEAR_INC_CONNDISPATCHER_H__

#include <vector>
#include <cstdint>
#include <string>

#include "listener.h"
#include "sysDefinitions.h"
#include "codes.h"
#include "ipHelper.h"
#include "macroFuncs.h"
#include "connFactory.h"
#include "connHandler.h"

namespace parrot
{
template <typename Conn,
          typename Cfg,
          template <typename, typename> class ConnFactory,
          template <typename> class ConnHandler>
class ConnDispatcher : public Listener
{
  public:
    ConnDispatcher(uint16_t listenPort, const std::string& listenIp)
        : Listener(listenPort, listenIp),
          _connHandlerVec(),
          _connHandlerVecIdx(0),
          _connUniqueIdIdx(0)
    {
    }

  public:
    inline void setConnHandler(std::vector<ConnHandler<Conn>*>&& connHandlerVec)
    {
        _connHandlerVec = std::move(connHandlerVec);
    }
    inline void setConnHandler(std::vector<ConnHandler<Conn>*>& connHandlerVec)
    {
        _connHandlerVec = connHandlerVec;
    }

  protected:
    eIoAction handleIoEvent() override
    {
        eIoAction act = getNotifiedAction();

        if (act == eIoAction::Remove)
        {
            return act;
        }

        PARROT_ASSERT(act == eIoAction::Read);

        eCodes code   = eCodes::ST_Init;
        int fd        = -1;
        uint16_t port = 0;
        IPHelper ipHelper;

        std::list<std::unique_ptr<Conn>> connList;
        std::unique_ptr<Conn> conn;

        do
        {
            code = doAccept(fd, ipHelper, port);

            conn = std::move(ConnFactory<Conn, Cfg>::getInstance()->create());
            conn->setFd(fd);
            conn->setNextAction(eIoAction::Read);
            conn->setRemoteAddr(ipHelper.getIPStr());
            conn->setRemotePort(port);
            conn->setUniqueKey(_connUniqueIdIdx++);
            connList.push_back(std::move(conn));
        } while (code == eCodes::ST_Ok);

        _connHandlerVec[_connHandlerVecIdx]->addConn(connList);
        _connHandlerVecIdx = (_connHandlerVecIdx + 1) % _connHandlerVec.size();

        return eIoAction::Read;
    }

  protected:
    std::vector<ConnHandler<Conn>*> _connHandlerVec;
    uint32_t _connHandlerVecIdx;
    uint64_t _connUniqueIdIdx;
};
}

#endif
