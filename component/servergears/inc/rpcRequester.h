#ifndef __COMPONENT_SERVERGEAR_INC_RPCREQUSETER_H__
#define __COMPONENT_SERVERGEAR_INC_RPCREQUSETER_H__

#include <string>

namespace parrot
{
class JobHandler;

struct RpcRequester
{
    RpcRequester(JobHandler* hdr,
                 const std::string& uid = "",
                 const std::string& sid = "");

    JobHandler* _rspHandler = nullptr;
    std::string _uid; // User id.
    std::string _sid; // Remote server id.

    std::string toString();
};
}

#endif
