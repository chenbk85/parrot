#ifndef __COMPONENT_SERVERGEAR_INC_SESSION_H__
#define __COMPONENT_SERVERGEAR_INC_SESSION_H__

#include <cstdint>
#include <string>
#include <unordered_map>

namespace parrot
{
struct Session
{
    virtual ~Session() = default;
    
    std::string _uid;
    std::string _frontSid;

    // <Server type, Server Id>
    std::unordered_map<std::string, std::string> _serverMap;

    void* _threadPtr = nullptr;
    void* _connPtr   = nullptr;

    std::string _ip;
};
}

#endif
