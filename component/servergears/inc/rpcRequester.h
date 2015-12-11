#ifndef __COMPONENT_SERVERGEAR_INC_RPCREQUSETER_H__
#define __COMPONENT_SERVERGEAR_INC_RPCREQUSETER_H__

#include <string>

namespace parrot
{
class JobHandler;

struct RpcRequester
{
    RpcRequester(JobHandler* hdr, const std::string& sessionStr = "");

    JobHandler* _rspHandler = nullptr;
    // Json to string. {"uid":"user1","sid":"front-srv-1"}
    // This string will be transferred to other servers.
    std::string _session;
    std::string toString();
};
}

#endif
