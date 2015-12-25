#ifndef __COMPONENT_SERVERGEAR_INC_CONNHANDLER_H__
#define __COMPONENT_SERVERGEAR_INC_CONNHANDLER_H__

#include <list>
#include <memory>
#include <mutex>

namespace parrot
{
template <typename Conn> class ConnHandler
{
  public:
    ConnHandler() = default;
    virtual ~ConnHandler() = default;

  public:
    void addConn(std::list<std::unique_ptr<Conn>>& connList);

  protected:
    virtual void afterAddNewConn() {}

  protected:
    std::mutex _newConnListLock;
    std::list<std::unique_ptr<RpcServerConn>> _newConnList;
};

template <typename Conn>
void ConnHandler<Conn>::addConn(std::list<std::unique_ptr<Conn>>& connList)
{
    _newConnListLock.lock();
    _newConnList.splice(_newConnList.end(), connList);
    _newConnListLock.unlock();

    afterAddConn();
}
}

#endif
