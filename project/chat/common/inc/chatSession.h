#ifndef __PROJECT_CHAT_FRONTSRV_INC_CHATSESSION_H__
#define __PROJECT_CHAT_FRONTSRV_INC_CHATSESSION_H__

namespace chat
{
class ChatSession
{
  public:
    ChatSession();
    parrot::JobHandler * getRpcRspHdr(parrot::RpcRequest* req);
    const string & getJsonStr() const;
  private:
    
};
}
#endif
