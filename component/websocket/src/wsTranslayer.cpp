#include <iostream>
#include <string>

#include "json.h"
#include "mtRandom.h"
#include "logger.h"
#include "wsPacket.h"
#include "ioEvent.h"
#include "wsHttpResponse.h"
#include "wsDecoder.h"
#include "wsConfig.h"
#include "macroFuncs.h"
#include "wsEncoder.h"
#include "wsTranslayer.h"

namespace parrot
{
WsTranslayer::WsTranslayer(IoEvent& io,
                           bool recvMasked,
                           bool sendMasked,
                           const WsConfig& cfg)
    : _state(RecvHttpHandshake),
      _io(io),
      _needRecvMasked(recvMasked),
      _needSendMasked(sendMasked),
      _pktList(),
      _httpRsp(),
      _wsDecoder(),
      _sendVec(_config._sendBuffLen),
      _needSendLen(0),
      _sentLen(0),
      _recvVec(_config._recvBuffLen),
      _rcvdLen(0),
      _wsEncoder(),
      _onOpenCb(),
      _onPacketCb(),
      _onErrorCb(),
      _random(nullptr),
      _config(cfg)
{
    _wsEncoder.reset(new WsEncoder(*this));
}

void WsTranslayer::setRandom(MtRandom *random)
{
    _random = random;
}

void WsTranslayer::registerOnOpenCb(std::function<void()>&& cb)
{
    _onOpenCb = std::move(cb);
}

void WsTranslayer::registerOnPacketCb(
    std::function<void(std::unique_ptr<WsPacket>&&)>&& cb)
{
    _onPacketCb = std::move(cb);
}

void WsTranslayer::registerOnErrorCb(std::function<void(eCodes)>&& cb)
{
    _onErrorCb = std::move(cb);
}

eCodes WsTranslayer::recvData()
{
    uint32_t rcvdLen = 0;
    eCodes code      = eCodes::ST_Ok;

    try
    {
        _io.recv(&_recvVec[_rcvdLen],
                 _recvVec.capacity() - _rcvdLen, rcvdLen);
        _rcvdLen += rcvdLen;
    }
    catch (std::system_error& e)
    {
        code = eCodes::ERR_Fail;
        LOG_WARN("WsTranslayer::recvData: Failed. Code is "
                 << e.code() << ". Msg is " << e.code().message()
                 << ". Remote is " << _io.getRemoteAddr() << ".");
    }

    return code;
}

eCodes WsTranslayer::sendData()
{
    uint32_t sentLen = 0;
    eCodes code = eCodes::ST_Ok;

    do
    {
        if (_needSendLen == 0 || _needSendLen == _sentLen)
        {
            _needSendLen = 0;
            code = _wsEncoder->loadBuff();
            if (code == eCodes::ST_Complete)
            {
                return code;
            }
            PARROT_ASSERT(_needSendLen > 0);
            _sentLen = 0;
        }

        try
        {
            // Send data here.
            _io.send(&_sendVec[0] + _sentLen, _needSendLen - _sentLen, sentLen);
        }
        catch (std::system_error& e)
        {
            code = eCodes::ERR_Fail;
            LOG_WARN("WsTranslayer::sendData: Failed. Code is "
                     << e.code() << ". Msg is " << e.code().message()
                     << ". Remote is " << _io.getRemoteAddr() << ".");
            break;
        }

        // Sent data successfully.
        _sentLen += sentLen;
        if (_sentLen == _needSendLen)
        {
            continue;
        }
        else
        {
            // The send buffer is full, we need to send data next time.
            code = eCodes::ST_NeedSend;
            break;
        }
    } while (true);

    return code;
}

void WsTranslayer::sendPacket(std::list<std::unique_ptr<WsPacket>>& pktList)
{
    for (auto& p : pktList)
    {
        _pktList.emplace_back(std::move(p));
    }
}

void WsTranslayer::sendPacket(std::unique_ptr<WsPacket>& pkt)
{
    _pktList.emplace_back(std::move(pkt));
}

bool WsTranslayer::isAllSent() const
{
    return _pktList.empty() && _needRecvMasked == _sentLen;
}

eIoAction WsTranslayer::work(eIoAction evt)
{
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
                _httpRsp.reset(new WsHttpResponse(*this));
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
                _rcvdLen = 0;
                _wsDecoder.reset(new WsDecoder(*this));
                
                // WebSocket is opened.
                _onOpenCb();
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
        {
            if (evt == eIoAction::Read)
            {
                code = recvData();
                if (code != eCodes::ST_Ok)
                {
                    return eIoAction::Remove;
                }

                code = _wsDecoder->parse();
                if (code == eCodes::ST_Complete)
                {
                    auto res = _wsDecoder->getResult();
                    if (res != eCodes::ST_Ok)
                    {
                        _onErrorCb(res);
                        return eIoAction::Write;
                    }

                    if (isAllSent())
                    {
                        return eIoAction::Read;
                    }
                    else
                    {
                        return eIoAction::Write;
                    }
                }
                else if (code == eCodes::ST_NeedRecv)
                {
                    return eIoAction::Read;
                }
                else
                {
                    PARROT_ASSERT(false);
                }
            }
            else if (evt == eIoAction::Write)
            {
                code = sendData();
                if (code == eCodes::ST_Complete)
                {
                    return eIoAction::Read;
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
            else
            {
                PARROT_ASSERT(false);
            }
        }
        break;

        default:
        {
            PARROT_ASSERT(false);
        }
        break;
    }

    PARROT_ASSERT(false);    
    return eIoAction::None;
}
}
