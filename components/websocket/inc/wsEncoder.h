#ifndef __COMPONENT_WEBSOCKET_INC_WSENCODER_H__
#define __COMPONENT_WEBSOCKET_INC_WSENCODER_H__

namespace parrot
{
    class WsEncoder
    {
      public:
        WsEncoder(std::vector<char> &sendVec, 
                  std::vector<char> &fragmentedSendVec);

        void toWsDataFrame(const WsPacket &pkt);

      private:

      private:
        std::vector<char>           _sendVec;
        std::vector<char>           _fragmentedSendVec;
    };
}

#endif
