#include <algorithm>
#include <iterator>

#include <openssl/sha1.h>

#include "ioEvent.h"
#include "wsParser.h"
#include "wsHttpResponse.h"
#include "wsTranslayer.h"
#include "logger.h"
#include "base64.h"
#include "stringHelper.h"
#include "digestHelper.h"

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

    eCodes WsTranslayer::recvData()
    {
        uint32_t rcvdLen = 0;
        eCodes code = eCodes::ST_Ok;

        try
        {
            _io.recv(&_recvVec[_rcvdLen], 
                              _recvVec.capacity() - _recvVec.size(), 
                              rcvdLen);
        }
        catch (std::system_error &e)
        {
            code = eCodes::ERR_Fail;
            LOG_WARN("WsTranslayer::recvData: Failed. Code is " << e.code()
                     << ". Msg is " << e.code().message() 
                     << ". Remote is " << _io.getRemoteAddr() << ".");
        }

        if (code == eCodes::ST_Ok)
        {
            _rcvdLen += rcvdLen;
        }
        
        return code;
    }

    eCodes WsTranslayer::sendData()
    {
        uint32_t sentLen = 0;
        eCodes code = eCodes::ST_Ok;

        try
        {
            _io.send(&_sendVec[_sentLen], 
                               _sendVec.size() - _sentLen, 
                               sentLen);
        }
        catch (std::system_error &e)
        {
            code = eCodes::ERR_Fail;
            LOG_WARN("WsTranslayer::sendData: Failed. Code is " << e.code()
                     << ". Msg is " << e.code().message() 
                     << ". Remote is " << _io.getRemoteAddr() << ".";
        }

        if (code == eCodes::ST_Ok)
        {
            _sentLen += sentLen;
            if (_sentLen == _sendVec.size())
            {
                _sendVec.resize(0);
                code == eCodes::ST_Complete;
            }
            else
            {
                code = eCodes::ST_NeedSend;
            }
        }
        
        return code;
    }

    eCodes WsTranslayer::sendPacket(
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

    eCodes WsTranslayer::sendPacket(const WsPacket &pkt)
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

    eIoAction WsTranslayer::work()
    {
        eIoAction act = eIoAction::None;
        eCodes code = eCodes::ST_Init;
        switch (_state)
        {
            case RecvHttpHandshake:
            {
                if (!_httpRsp)
                {
                    _httpRsp.reset(new WsHttpResponse(*this));
                }

                code = _httpRsp->work();
                if (code == eCodes::ST_NeedRecv)
                {
                    return eIoAction::Read;
                }
                else (code == eCodes::ST_Complete)
                {
                    _state = SendHttpHandshake;
                    return work();
                }
                else
                {
                    PARROT_ASSERT(false);
                }
            }
            break;

            case SendHttpHandshake:
            {
                code = sendData();
                if (code == eCodes::ST_Complete)
                {
                    _state = RecvDataFrame;
                    return work();
                }
                else if (code == eCodes::ST_NeedSend)
                {
                    return eIoAction::Write;
                }
                else
                {
                    return eIoAction::Remove;
                }
            }
            break;

            case RecvDataFrame:
                recvData();
                // No break;

            case SendDataFrame:
                act = sendData();
                break;

            default:
                PARROT_ASSERT(false);
                break;
        }

        return act;
    }
}
