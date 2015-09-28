#ifndef __COMPONENT_WEBSOCKET_INC_WSTRANSLAYER_H__
#define __COMPONENT_WEBSOCKET_INC_WSTRANSLAYER_H__

#include <list>
#include <memory>
#include <algorithm> // std::copy
#include <vector>
#include <cstdint>
#include <functional>

#include "logger.h"
#include "codes.h"
#include "wsPacket.h"
#include "ioEvent.h"
#include "wsHttpResponse.h"
#include "wsParser.h"
#include "wsConfig.h"
#include "macroFuncs.h"

namespace parrot
{
template <typename WsIo> class WsTranslayer
{
  private:
    enum eTranslayerState
    {
        RecvHttpHandshake,
        SendHttpHandshake,
        WsConnected,
        Closing
    };

  public:
    WsTranslayer(WsIo& io, bool needRecvMasked, const WsConfig& cfg)
        : _state(RecvHttpHandshake),
          _wsIo(io),
          _needRecvMasked(needRecvMasked),
          _pktList(),
          _httpRsp(),
          _wsParser(),
          _sendVec(),
          _sentLen(0),
          _recvVec(),
          _config(cfg)
    {
        _sendVec.reserve(_config._sendBuffLen);
        _recvVec.reserve(_config._recvBuffLen);
    }

    ~WsTranslayer() = default;
    WsTranslayer(const WsTranslayer&) = delete;
    WsTranslayer& operator=(const WsTranslayer&) = delete;

  public:
    void sendPacket(std::list<std::unique_ptr<WsPacket>>& pktList);
    void sendPacket(std::unique_ptr<WsPacket>& pkt);
    eIoAction work(eIoAction evt);
    void close();

  private:
    eCodes recvData();
    eCodes sendData();

  private:
    eTranslayerState _state;
    WsIo& _wsIo;
    bool _needRecvMasked;
    std::list<std::unique_ptr<WsPacket>> _pktList;

    std::unique_ptr<WsHttpResponse> _httpRsp;
    std::unique_ptr<WsParser> _wsParser;

    std::vector<char> _sendVec;
    uint32_t _sentLen;

    std::vector<char> _recvVec;
    const WsConfig& _config;
};

template <typename WsIo> eCodes WsTranslayer<WsIo>::recvData()
{
    uint32_t rcvdLen = 0;
    eCodes code = eCodes::ST_Ok;

    // TODO: Need receive more.
    try
    {
        _wsIo.recv(&_recvVec[_recvVec.size()],
                   _recvVec.capacity() - _recvVec.size(), rcvdLen);
    }
    catch (std::system_error& e)
    {
        code = eCodes::ERR_Fail;
        LOG_WARN("WsTranslayer::recvData: Failed. Code is "
                 << e.code() << ". Msg is " << e.code().message()
                 << ". Remote is " << _wsIo.getRemoteAddr() << ".");
    }

    return code;
}

template <typename WsIo> eCodes WsTranslayer<WsIo>::sendData()
{
    uint32_t sentLen = 0;
    eCodes code = eCodes::ST_Ok;

    try
    {
        _wsIo.send(&_sendVec[_sentLen], _sendVec.size() - _sentLen, sentLen);
    }
    catch (std::system_error& e)
    {
        code = eCodes::ERR_Fail;
        LOG_WARN("WsTranslayer::sendData: Failed. Code is "
                 << e.code() << ". Msg is " << e.code().message()
                 << ". Remote is " << _wsIo.getRemoteAddr() << ".");

        if (code == eCodes::ST_Ok)
        {
            _sentLen += sentLen;
            if (_sentLen == _sendVec.size())
            {
                _sendVec.resize(0);
                code = eCodes::ST_Complete;
            }
            else
            {
                code = eCodes::ST_NeedSend;
            }
        }
    }
    return code;
}

template <typename WsIo>
void WsTranslayer<WsIo>::sendPacket(
    std::list<std::unique_ptr<WsPacket>>& pktList)
{
    // TODO: fragment packet.
    std::unique_ptr<std::vector<char>> buf;
    for (auto it = pktList.begin(); it != pktList.end(); ++it)
    {
        buf = std::move((*it)->toBuffer());
        std::copy_n(&(*buf.get())[0], (buf.get())->size(),
                    std::back_inserter(_sendVec));
    }

    if (_sendVec.size() > _config._sendBuffLen)
    {
        LOG_WARN("WsTranslayer::sendPacket: _sendVec reallocated. "
                 "Try to change the SEND_BUFF_LEN to a bigger value. "
                 "Current size is "
                 << _sendVec.size() << ".");
    }
}

template <typename WsIo>
void WsTranslayer<WsIo>::sendPacket(std::unique_ptr<WsPacket>& pkt)
{
    auto buf = std::move(pkt->toBuffer());
    std::copy_n(&(*buf.get())[0], (buf.get())->size(),
                std::back_inserter(_sendVec));

    if (_sendVec.size() > _config._recvBuffLen)
    {
        LOG_WARN("WsTranslayer::sendPacket: _sendVec reallocated. "
                 "Try to change the SEND_BUFF_LEN to a bigger value. "
                 "Current size is "
                 << _sendVec.size() << ".");
    }
}

template <typename WsIo> eIoAction WsTranslayer<WsIo>::work(eIoAction evt)
{
    eIoAction act = eIoAction::None;
    eCodes code = eCodes::ST_Init;
    switch (_state)
    {
    case eTranslayerState::RecvHttpHandshake:
    {
        if (evt != eIoAction::Read)
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
                                              _wsIo.getRemoteAddr(), _config));
        }

        code = _httpRsp->work();
        if (code == eCodes::ST_NeedRecv)
        {
            return eIoAction::Read;
        }
        else if (code == eCodes::ST_Complete)
        {
            _state = SendHttpHandshake;
            return work(eIoAction::Write);
        }
        else
        {
            PARROT_ASSERT(false);
        }
    }
    break;

    case eTranslayerState::SendHttpHandshake:
    {
        if (evt != eIoAction::Write)
        {
            PARROT_ASSERT(false);
        }

        code = sendData();
        if (code == eCodes::ST_Complete)
        {
            if (_httpRsp->getResult() != eCodes::HTTP_SwitchingProtocols)
            {
                return eIoAction::Remove;
            }

            _httpRsp.reset(nullptr);
            _state = WsConnected;
            using namespace std::placeholders;
            auto cb = std::bind(&WsIo::onData, &_wsIo, _1, _2, _3);
            _wsParser.reset(new WsParser(cb, _recvVec, _wsIo.getRemoteAddr(),
                                         _needRecvMasked, _config));
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
        if (evt == eIoAction::Read)
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
                    // TODO:
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
