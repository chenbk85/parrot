#ifndef __COMPONENT_WEBSOCKET_INC_SESSION_H__
#define __COMPONENT_WEBSOCKET_INC_SESSION_H__

#include <cstdint>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

namespace parrot
{
struct Session
{
  public:
    Session();
    virtual ~Session() = default;

  public:
    virtual std::string toString() const;

  public:
    bool _isBound;
    std::string _uid;
    std::string _frontSid;

    // <Server type, Server Id>
    std::unordered_map<std::string, std::string> _serverMap;

    void* _backThreadPtr;
    void* _frontThreadPtr;
    uint64_t _connUniqueId;

    std::string _ip;
};
}

#endif
