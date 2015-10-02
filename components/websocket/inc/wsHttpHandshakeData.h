#ifndef __COMPONENTS_WEBSOCKET_INC_WSHTTPPACKET_H__
#define __COMPONENTS_WEBSOCKET_INC_WSHTTPPACKET_H__

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
    std::vector<char> _body;
};
}

#endif
