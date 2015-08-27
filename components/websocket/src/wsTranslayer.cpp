#include "wsTranslayer.h"


namespace parrot
{
    WsTranslayer::WsTranslayer():
        _sendVec(SEND_BUFF_LEN),
        _sentLen(0),
        _recvVec(RECV_BUFF_LEN),
        _rcvdLen(0),
        _needRecvLen(0),
        _io(nullptr)
    {
        _sendVec.resize(0);
        _recvVec.resize(0);
    }

    WsTranslayer::~WsTranslayer()
    {
    }

    Codes WsTranslayer::recvData()
    {
        uint32_t rcvdLen = 0;
        Codes code = _io->recv(&_recvVec[_rcvdLen], 
                               _recvVec.capacity() - _recvVec.size(), 
                               rcvdLen);

        if (code == Codes::ST_Ok)
        {
            _rcvdLen += rcvdLen;
            code = _parser->parse();
        }
        
        return code;
    }

    void WsTranslayer::sendData()
    {
        uint32_t sentLen = 0;
        Codes code = _io->send(&_sendVec[_sentLen], 
                               _sendVec.size() - _sentLen, 
                               sentLen);

        if (code == Codes::ST_Ok)
        {
            _sentLen += sentLen;
            if (_sentLen == _sendVec.size())
            {
                _sendVec.resize(0);
            }
        }
        
        return code;        
    }

    Codes WsTranslayer::sendPacket(const WsPacket &pkt)
    {
        
    }
}
