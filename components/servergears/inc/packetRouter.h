#ifndef __COMPONENT_SERVERGEAR_INC_PACKETROUTER_H__
#define __COMPONENT_SERVERGEAR_INC_PACKETROUTER_H__

namespace parrot
{
class PacketRouter
{
  public:
    void * getDefaultHandler();

  private:
    MtRandom _random;
};
}


#endif
