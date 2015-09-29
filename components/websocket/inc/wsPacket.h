#ifndef __COMPONENT_WEBSOCKET_INC_WSPACKET_H__
#define __COMPONENT_WEBSOCKET_INC_WSPACKET_H__

#include <memory>
#include <vector>
#include <cstdint>

#include "wsDefinition.h"
#include "codes.h"

namespace parrot
{
class Json;

class WsPacket
{
  public:
    WsPacket();
    ~WsPacket() = default;
    WsPacket(const WsPacket& pkt) = delete;
    WsPacket& operator=(const WsPacket& pkt) = delete;
    WsPacket(WsPacket&& pkt) = default;
    WsPacket& operator=(WsPacket&& pkt) = default;

  public:
    bool isPacketUndecoded() const;
    bool isControl() const;
    
    void setRoute(uint64_t route);
    void setJson(Json&& json);
    void setBinary(std::vector<char>&& bin);
    void setRawData(std::vector<char>> &&orig);
    void setOpCode(eOpCode opCode);
    void setClose(eCodes code, std::string &&reason = "");

    eOpCode getOpCode() const;
    eCodes getCloseCode() const;
    const string & getCloseReason() const;
    
    uint64_t getRoute() const;
    const std::vector<char>& getBinary() const;
    const Json& getJson() const;
    const std::vector<char>& getRawData() const;

  private:
    eOpCode _opCode;
    eCodes _closeCode;
    std::string _reason;
    std::unique_ptr<Json> _json;
    std::std::vector<char> _bin;
    std::std::vector<char> _raw;
    uint64_t _route;
};
}

#endif
