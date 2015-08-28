#include "ioEvent.h"
#include "wsPacket.h"
#include "wsParser.h"
#include "wsTranslayer.h"

#include <algorithm>
#include <iterator>

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

    Codes WsTranslayer::sendData()
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

    Codes WsTranslayer::sendPacket(
        std::list<std::shared_ptr<WsPacket>> &pktList)
    {
        for (auto it = pktList.begin(); it != pktList.end(); ++it)
        {
            std::copy_n((*it)->getBuffer(), (*it)->length(), 
                        std::back_inserter(_sendVec));
        }

        if (_sendVec.size() > SEND_BUFF_LEN)
        {
            LOG_WARN("WsTranslayer::sendPacket: _sendVec reallocated. "
                     "Try to change the SEND_BUFF_LEN to a bigger value. "
                     "Current size is " << _sendVec.size() << ".");
        }
    }

    Codes WsTranslayer::sendPacket(const WsPacket &pkt)
    {
        std::copy_n(pkt.getBuffer(), pkt.length(), 
                    std::back_inserter(_sendVec));

        if (_sendVec.size() > SEND_BUFF_LEN)
        {
            LOG_WARN("WsTranslayer::sendPacket: _sendVec reallocated. "
                     "Try to change the SEND_BUFF_LEN to a bigger value. "
                     "Current size is " << _sendVec.size() << ".");
        }
    }
}
