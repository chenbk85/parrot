#ifndef __COMPONENT_WEBSOCKET_INC_WSENCODER_H__
#define __COMPONENT_WEBSOCKET_INC_WSENCODER_H__

#include <vector>

namespace parrot
{
class WsConfig;
class WsPacket;

class WsEncoder
{
  public:
    WsEncoder(std::vector<char>& sendVec,
              std::vector<char>& fragmentedSendVec,
              const WsConfig& cfg);

  public:
    void toWsDataFrame(const WsPacket& pkt);

  private:
  private:
    std::vector<char> _sendVec;
    std::vector<char> _fragmentedSendVec;
    const WsConfig& _config;
    bool _needMask;
};
}

#endif
