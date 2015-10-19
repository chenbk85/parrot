#ifndef __COMPONENT_SERVERGEAR_INC_PACKETROUTER_H__
#define __COMPONENT_SERVERGEAR_INC_PACKETROUTER_H__

namespace parrot
{
class PacketRouter
{
  public:
    bool handle(WsServerConn *connPtr, std::unique_ptr<WsPacket> &&pkt);
    bool isLocalPacket(uint64_t route);

  private:
    void * getDefaultHandler();
        
  private:
    MtRandom _random;
};
}


#endif
