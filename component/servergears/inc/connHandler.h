#ifndef __COMPONENT_SERVERGEAR_INC_CONNHANDLER_H__
#define __COMPONENT_SERVERGEAR_INC_CONNHANDLER_H__

#include <list>
#include <memory>
#include <mutex>

namespace parrot
{
template <typename Conn>
class ConnHandler
{
  public:
    virtual void addConn(std::list<std::unique_ptr<Conn>> &connList) = 0;
};
}

#endif
