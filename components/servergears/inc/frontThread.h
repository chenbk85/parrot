#ifndef __COMPONENT_SERVERGEAR_INC_FRONTTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_FRONTTHREAD_H__

#include <unordered_map>
#include <memory>
#include <mutex>
#include <list>
#include <cstdint>

#include "poolThread.h"

namespace parrot
{
struct Config;
class EventNotifier;
class WsServerConn;
    
class FrontThread : public PoolThread
{
  public:
    FrontThread();

  public:
    void setConfig(const Config* cfg);
    void addConn(std::list<std::shared_ptr<WsServerConn>> &connList);
    void addConn(std::shared_ptr<WsServerConn> &&conn);

  protected:
    void beforeStart() override;
    void run() override();

  private:
    std::mutex _newConnListLock;
    std::list<std::shared_ptr<WsServerConn>> _newConnList;

    std::unordered_map<uint64_t, std::shared_ptr<WsServerConn>> _connMap;

    std::unique_ptr<EventNotifier> _notifier;
    
    const Config* _config;
};
}

#endif
