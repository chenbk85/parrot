#ifndef __COMPONENT_WEBSOCKET_INC_WSHTTPPACKET_H__
#define __COMPONENT_WEBSOCKET_INC_WSHTTPPACKET_H__

#include <vector>
#include <string>
#include <unordered_map>

namespace parrot
{
class WsHttpHandshakeData
{
  public:
    using HeaderDic = std::unordered_map<std::string, std::string>;
    HeaderDic _header;
    std::vector<unsigned char> _body;
};
}

#endif
