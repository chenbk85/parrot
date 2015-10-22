#ifndef __COMPONENTS_SERVERGEAR_INC_PACKETROUTER_H__
#define __COMPONENTS_SERVERGEAR_INC_PACKETROUTER_H__

namespace parrot
{
class PacketRouter
{
    using DefaultPktHandler =
        std::function<void(
            std::list<std::pair<uint64_t, std::unique_ptr<WsPacket>>>)>;
  public:
    bool isLocalPacket(uint64_t route);
    void registerDefaultPktHandler(DefautPktHandler &&hdr);
    void callDefaultPktHandler(
        std::list<std::pair<uint64_t, std::unique_ptr<WsPacket>>> &&pktList);
        
  private:
    DefaultRouter _defaultRouter;
    MtRandom _random;
};
}


#endif
