#ifndef __COMPONENT_SERVERGEAR_INC_RPCSESSION_H__
#define __COMPONENT_SERVERGEAR_INC_RPCSESSION_H__

#include <string>

namespace parrot
{
class RpcSession
{
  public:
    RpcSession() = default;
    
  public:
    const std::string& getRemoteSid() const;
    void setRemoteSid(const std::string &sid);

    std::string toString() const;
    
  private:
    std::string _remoteSid;
};
}

#endif
