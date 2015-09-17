#ifndef __COMPONENT_WEBSOCKET_INC_WSTRANSLAYER_H__
#define __COMPONENT_WEBSOCKET_INC_WSTRANSLAYER_H__

#include <list>
#include <memory>

#include <vector>
#include <cstdint>

#include "logger.h"
#include "codes.h"

namespace parrot
{
    template<typename WsIo> class WsTranslayer
    {
      public:
        enum
        {
            kHttpHandshakeLen = 8192,
            kSendBuffLen = 65536,
            kRecvBuffLen = 65536,
            kRecvMaxLen = 1 << 20 // 1 MB
        };

      private:
        enum eTranslayerState
        {
            RecvHttpHandshake,
            SendHttpHandshake,
            WsConnected,
            Closing
        };

      public:
        WsTranslayer(WsIo &io, bool needRecvMasked) :
            _state(RecvHttpHandshake),
            _wsIo(io),
            _needRecvMasked(needRecvMasked),
            _pktList(),
            _httpRsp(),
            _wsParser(),
            _sendVec(),
            _sentLen(0),
            _recvVec()
        {
            _sendVec.reserve(kSendBuffLen);
            _recvVec.reserve(kRecvBuffLen);
        }

        ~WsTranslayer() = default;
        WsTranslayer(const WsTranslayer &) = delete;
        WsTranslayer& operator=(const WsTranslayer &) = delete;

      public:
        eCodes sendPacket(std::list<std::unique_ptr<WsPacket>> &pktList);
        eCodes sendPacket(const WsPacket &pkt);
        eIoAction work(IoEvent evt);
        void close();

      private:
        eCodes recvData();
        eCodes sendData();
        
      private:
        eTranslayerState                         _state;
        WsIo &                                   _wsIo;
        bool                                     _needRecvMasked;
        std::list<std::unique_ptr<Packet>>       _pktList;

        std::unique_ptr<WsHttpResponse>          _httpRsp;
        std::unique_ptr<WsParser>                _wsParser;

        std::vector<char>                        _sendVec;
        uint32_t                                 _sentLen;

        std::vector<char>                        _recvVec;
    };

    template<typename WsIo>
    eCodes WsTranslayer::recvData()
    {
        uint32_t rcvdLen = 0;
        eCodes code = eCodes::ST_Ok;

        // TODO: Need receive more.
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

    template<typename WsIo>
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
                     << ". Remote is " << _io.getRemoteAddr() << ".");

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
    }

    template<typename WsIo>
    eCodes WsTranslayer::sendPacket(
        std::list<std::unique_ptr<WsPacket>> &pktList)
    {
        // TODO: fragment packet.
        std::unique_ptr<std::vector<char>> buf;
        for (auto it = pktList.begin(); it != pktList.end(); ++it)
        {
            buf = std::move((*it)->toBuffer());
            std::copy_n(&(*buf.get())[0], (buf.get())->length, 
                        std::back_inserter(_sendVec));
        }

        if (_sendVec.size() > SEND_BUFF_LEN)
        {
            LOG_WARN("WsTranslayer::sendPacket: _sendVec reallocated. "
                     "Try to change the SEND_BUFF_LEN to a bigger value. "
                     "Current size is " << _sendVec.size() << ".");
        }
    }

    template<typename WsIo>
    eCodes WsTranslayer::sendPacket(std::unique_ptr<WsPacket> &&pkt)
    {
        auto buf = std::move((*it)->toBuffer());
        std::copy_n(&(*buf.get())[0], (buf.get())->length, 
                    std::back_inserter(_sendVec));

        if (_sendVec.size() > SEND_BUFF_LEN)
        {
            LOG_WARN("WsTranslayer::sendPacket: _sendVec reallocated. "
                     "Try to change the SEND_BUFF_LEN to a bigger value. "
                     "Current size is " << _sendVec.size() << ".");
        }
    }

    template<typename WsIo>
    eIoAction WsTranslayer::work(IoEvent evt)
    {
        eIoAction act = eIoAction::None;
        eCodes code = eCodes::ST_Init;
        switch (_state)
        {
            case eTranslayerState::RecvHttpHandshake:
            {
                if (evt != IoEvent::Read)
                {
                    PARROT_ASSERT(false);
                }

                code = recvData();
                if (code != eCodes::ST_Ok)
                {
                    return eIoAction::Remove;
                }

                if (!_httpRsp)
                {
                    _httpRsp.reset(new WsHttpResponse(_recvVec, _sendVec,
                                                      _remoteIp, _config));
                }

                code = _httpRsp->work();
                if (code == eCodes::ST_NeedRecv)
                {
                    return eIoAction::Read;
                }
                else if (code == eCodes::ST_Complete)
                {
                    _state = SendHttpHandshake;
                    return work(IoEvent::Write);
                }
                else
                {
                    PARROT_ASSERT(false);
                }
            }
            break;

            case eTranslayerState::SendHttpHandshake:
            {
                if (evt != IoEvent::Write)
                {
                    PARROT_ASSERT(false);
                }

                code = sendData();
                if (code == eCodes::ST_Complete)
                {
                    if (_httpRsp->getResult() != 
                        eCodes::HTTP_SwitchingProtocols)
                    {
                        return eIoAction::Remove;
                    }

                    _httpRsp.reset(nullptr);
                    _io.onOpen();
                    _state = WsConnected;
                    _wsParser.reset(new WsParser(_io, _recvVec, _remoteIp, 
                                                 _needRecvMasked));
                    return work(eIoAction::Read);
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

            case eTranslayerState::WsConnected:
                if (evt == IoEvent::Read)
                {
                    code = recvData();
                    if (code != eCodes::ST_Ok)
                    {
                        return eIoAction::Remove;
                    }

                    code = _wsParser->parse();

                    if (code == eCodes::ST_Complete)
                    {
                        if (_wsParser->getResult() != eCodes::ST_Ok)
                        {
                            _state = eTranslayerState::Closing;
                            return work();
                        }
                    }
                    else
                    {
                        PARROT_ASSERT(false);
                    }
                }

                if (_sendVec.empty())
                {
                    return eIoAction::Read;
                }

                sendData();

            default:
                PARROT_ASSERT(false);
                break;
        }

        return act;
    }
}

#endif
