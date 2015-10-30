#ifndef __COMPONENT_SERVERGEAR_INC_CONNACCEPTOR_H__
#define __COMPONENT_SERVERGEAR_INC_CONNACCEPTOR_H__

#include <list>
#include <memory>
#include <mutex>

namespace parrot
{
template <typename Conn>
class ConnAcceptor
{
  public:
    virtual ~ConnAcceptor() = default;

  public:
    virtual void addConn(std::list<std::unique_ptr<Conn>> &connList)
    {
        _connListLock.lock();
        _connList.splice(_connList.end(), connList);
        _connListLock.unlock();
    }

  protected:
    std::mutex _connListLock;
    std::list<std::unique_ptr<Conn>> _connList;
};
}

#endif
