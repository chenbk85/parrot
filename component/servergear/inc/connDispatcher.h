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
#include "connManager.h"
#include "urlParser.h"

namespace parrot
{
template <typename Conn,
          typename Cfg,
          template <typename, typename> class ConnFactory,
          template <typename> class ConnManager>
class ConnDispatcher : public Listener
{
  public:
    ConnDispatcher(uint16_t listenPort, const std::string& listenIp)
        : Listener(listenPort, listenIp),
          _connManagerVec(),
          _connManagerVecIdx(0),
          _connUniqueIdIdx(0)
    {
    }

  public:
    inline void setConnManager(std::vector<ConnManager<Conn>*>&& connManagerVec)
    {
        _connManagerVec = std::move(connManagerVec);
    }
    inline void setConnHandler(std::vector<ConnManager<Conn>*>& connManagerVec)
    {
        _connManagerVec = connManagerVec;
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

            if (code != eCodes::ST_Ok)
            {
                break;
            }

            conn = std::move(ConnFactory<Conn, Cfg>::getInstance()->create());
            conn->setFd(fd);
            conn->setRemoteAddr(ipHelper.getIPStr());
            conn->setRemotePort(port);
            conn->setUrlInfo(getUrlInfo());
            connList.push_back(std::move(conn));
        } while (code == eCodes::ST_Ok);

        if (!connList.empty())
        {
            _connManagerVec[_connManagerVecIdx]->addConn(connList);
            _connManagerVecIdx =
                (_connManagerVecIdx + 1) % _connManagerVec.size();
        }

        return eIoAction::Read;
    }

  protected:
    std::vector<ConnManager<Conn>*> _connManagerVec;
    uint32_t _connManagerVecIdx;
    uint64_t _connUniqueIdIdx;
};
}

#endif
