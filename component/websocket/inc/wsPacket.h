#ifndef __COMPONENT_WEBSOCKET_INC_WSPACKET_H__
#define __COMPONENT_WEBSOCKET_INC_WSPACKET_H__

#include <string>
#include <memory>
#include <vector>
#include <cstdint>

#include "json.h"
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
    bool isRaw() const;
    bool isControl() const;

    void setRoute(uint64_t route);
    void setReqId(uint64_t reqId);
    void setConnId(uint64_t connId);
    void setJson(std::unique_ptr<Json>&& json);
    void setSysJson(std::unique_ptr<Json>&& json);
    void setBinary(std::vector<unsigned char>&& bin);
    void setPacket(eOpCode opCode, std::vector<unsigned char>&& payload);
    void setOpCode(eOpCode opCode);
    void setClose(eCodes code, std::string&& reason = "");

    eOpCode getOpCode() const;
    eCodes getCloseCode() const;
    const std::string& getCloseReason() const;

    uint64_t getRoute() const;
    uint64_t getReqId() const;
    uint64_t getConnId() const;
    const std::vector<unsigned char>& getBinary() const;
    const Json* getJson() const;
    const Json* getSysJson() const;

    const std::vector<unsigned char>& getPayload() const;

    bool decode();

  private:
    bool decodeSysData();
    bool decodeBinary();
    bool decodeClose();
    bool decodeItemMeta(std::vector<unsigned char>::const_iterator& it,
                        ePayloadItem& item,
                        uint64_t& itemLen);
  private:
    eOpCode _opCode;
    eCodes _closeCode;
    std::string _reason;
    std::unique_ptr<Json> _json;
    std::unique_ptr<Json> _sysJson;
    std::vector<unsigned char> _bin;
    std::vector<unsigned char> _jsonBin;
    std::vector<unsigned char> _payload;
    uint64_t _route;
    uint64_t _reqId;
    uint64_t _connId;
    bool _decoded;
    bool _decodeResult;
};
}

#endif
