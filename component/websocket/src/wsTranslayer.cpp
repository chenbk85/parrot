#include "json.h"
#include "mtRandom.h"
#include "logger.h"
#include "wsPacket.h"
#include "ioEvent.h"
#include "wsHttpResponse.h"
#include "wsParser.h"
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
      _wsParser(),
      _sendVec(),
      _sendFragmentedVec(),
      _sentLen(0),
      _recvVec(),
      _wsEncoder(),
      _onPacketCb(),
      _onErrorCb(),
      _random(nullptr),
      _config(cfg)
{
    _sendVec.reserve(_config._sendBuffLen);
    _recvVec.reserve(_config._recvBuffLen);

    // TODO:
    _wsEncoder.reset(new WsEncoder(_sendVec, _sendFragmentedVec, _config,
                                   *_random, _needSendMasked));
}

void WsTranslayer::setRandom(MtRandom *random)
{
    _random = random;
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
    eCodes code = eCodes::ST_Ok;

    try
    {
        if (_recvVec.capacity() == _recvVec.size())
        {
            PARROT_ASSERT(false);
        }
        
        _io.recv(&_recvVec[_recvVec.size()],
                   _recvVec.capacity() - _recvVec.size(), rcvdLen);
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

std::vector<char>* WsTranslayer::prepareDataToSend()
{
    do
    {
        if (!_sendVec.empty())
        {
            return &_sendVec;
        }

        if (!_sendFragmentedVec.empty())
        {
            return &_sendFragmentedVec;
        }

        if (_pktList.empty())
        {
            return nullptr;
        }

        auto& pkt = *(_pktList.begin());
        _wsEncoder->encode(*pkt);
        _pktList.pop_front();
    } while (true);
}

eCodes WsTranslayer::sendData()
{
    uint32_t sentLen = 0;
    eCodes code = eCodes::ST_Ok;

    do
    {
        // Get data to send.
        std::vector<char>* vp = prepareDataToSend();
        if (!vp)
        {
            // All data has been sent.
            return eCodes::ST_Complete;
        }

        try
        {
            // Send data here.
            _io.send(&((*vp)[_sentLen]), vp->size() - _sentLen, sentLen);
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
        if (_sentLen == vp->size())
        {
            // The packet has been sent. Reset and load next packet.
            vp->clear();
            sentLen = 0;
            _sentLen = 0;
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
    return _sendFragmentedVec.empty() && _sendVec.empty() && _pktList.empty();
}

eIoAction WsTranslayer::work(eIoAction evt)
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
                _httpRsp.reset(new WsHttpResponse(
                    _recvVec, _sendVec, _io.getRemoteAddr(), _config));
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
                _wsParser.reset(new WsParser(std::move(_onPacketCb), _recvVec,
                                             _io.getRemoteAddr(),
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
        {
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
                    auto res = _wsParser->getResult();
                    if (res != eCodes::ST_Ok)
                    {
                        _onErrorCb(res);
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

    return act;
}
}
