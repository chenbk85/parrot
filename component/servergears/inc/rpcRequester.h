#ifndef __COMPONENT_SERVERGEAR_INC_RPCREQUSETER_H__
#define __COMPONENT_SERVERGEAR_INC_RPCREQUSETER_H__

#include <string>

namespace parrot
{
class JobHandler;

template <typename Sess>
struct RpcRequester
{
    RpcRequester(JobHandler* hdr, const std::string& sessionStr = "");
    JobHandler* _rspHandler = nullptr;
    std::shared_ptr<Sess> _session;
    std::string toString();
};
}

#endif
