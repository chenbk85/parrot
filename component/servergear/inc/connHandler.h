#ifndef __COMPONENT_SERVERGEAR_INC_CONNHANDLER_H__
#define __COMPONENT_SERVERGEAR_INC_CONNHANDLER_H__

#include <list>
#include <memory>
#include <mutex>

#include "eventNotifier.h"

namespace parrot
{
template <typename Conn> class ConnHandler
{
  public:
    ConnHandler();
    virtual ~ConnHandler() = default;

  public:
    void addConn(std::list<std::unique_ptr<Conn>>& connList);

  protected:
    void setEventNotifier(EventNotifier* n);

  protected:
    std::mutex _newConnListLock;
    std::list<std::unique_ptr<Conn>> _newConnList;

  private:
    EventNotifier* _baseNotifier;
};

template <typename Conn>
ConnHandler<Conn>::ConnHandler()
    : _newConnListLock(), _newConnList(), _baseNotifier(nullptr)
{
}

template <typename Conn>
void ConnHandler<Conn>::setEventNotifier(EventNotifier* n)
{
    _baseNotifier = n;
}

template <typename Conn>
void ConnHandler<Conn>::addConn(std::list<std::unique_ptr<Conn>>& connList)
{
    _newConnListLock.lock();
    _newConnList.splice(_newConnList.end(), connList);
    _newConnListLock.unlock();

    _baseNotifier->stopWaiting();
}
}

#endif
