#ifndef __COMPONENT_SERVERGEAR_INC_CONNDISPATCHER_H__
#define __COMPONENT_SERVERGEAR_INC_CONNDISPATCHER_H__

#include "listener.h"

namespace parrot
{
template <typename Conn,
          template <typename> typename ConnFactory,
          template <typename> typename ConnAcceptor>
class ConnDispatcher : public Listener
{
  public:
    ConnDispatcher(uint16_t listenPort, const std::string& listenIp);

  public:
    void setConnAcceptor(std::vector<ConnAcceptor<Conn>*>&& jobHandler);
    void setConnAcceptor(std::vector<ConnAcceptor<Conn>*>& jobHandler);

  protected:
    eIoAction handleIoEvent() override;

  protected:
    std::vector<ConnAcceptor<Conn>*> _connAcceptorVec;
    uint32_t _jobHandlerVecIdx;
    uint64_t _connUniqueIdIdx;
};

template <typename Conn,
          template <typename> typename ConnFactory,
          template <typename> typename ConnAcceptor>
ConnDispatcher::ConnDispatcher(uint16_t listenPort, const std::string& listenIp)
    : Listener(listenPort, listenIp),
      _connAcceptorVec(),
      _connAcceptorVecIdx(0),
      _connUniqueIdIdx(0)
{
}

template <typename Conn,
          template <typename> typename ConnFactory,
          template <typename> typename ConnAcceptor>
void ConnDispatcher<ConnFactory>::setConnAcceptor<Conn>(
    std::vector<ConnAcceptor<Conn>*>&& connAcceptor)
{
    _connAcceptorVec = std::move(jobhandler);
}

template <typename Conn,
          template <typename> typename ConnFactory,
          template <typename> typename ConnAcceptor>
void ConnDispatcher<ConnFactory>::setConnAcceptor<Conn>(
    std::vector<ConnAcceptor<Conn>*>& connAcceptor)
{
    _connAcceptorVec = jobhandler;
}

template <typename Conn,
          template <typename> typename ConnFactory,
          template <typename> typename ConnAcceptor>
eIoAction ConnDispatcher<ConnFactory>::handleIoEvent()
{
    eIoAction act = getNotifiedAction();

    if (act == eIoAction::Remove)
    {
        return act;
    }

    PARROT_ASSERT(act == eIoAction::Read);

    eCodes code   = ST_Init;
    int fd        = -1;
    uint16_t port = 0;
    IPHelper ipHelper;

    std::list<std::unique_ptr<Conn>> connList;
    std::unique_ptr<Conn> conn;

    do
    {
        code = doAccept(fd, ipHelper, port);

        conn = std::move(ConnFactory::getInstance()->create()));
        conn->setFd(fd);
        conn->setNextAction(eIoAction::Read);
        conn->setRemoteAddr(ipHelper.getIpStr());
        conn->setUniqueKey(_connUniqueIdIdx++);
        connList.push_back(std::move(conn));
    } while (code == eCodes::ST_Ok);

    _connAcceptorVec[_connAcceptorVecIdx]->addConn(connList);
    _connAcceptorVecIdx = (_connAcceptorVecIdx + 1) % _connAcceptorVec.size();

    return eIoAction::Read;
}
}

#endif
