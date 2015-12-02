#include "codes.h"
#include "ioEvent.h"
#include "macroFuncs.h"
#include "wsPacket.h"
#include "wsClientTrans.h"
#include "wsDecoder.h"
#include "wsEncoder.h"

namespace parrot
{
WsClientTrans::WsClientTrans(IoEvent& io,
                             bool recvMasked,
                             bool sendMasked,
                             const WsConfig& cfg)
    : WsTranslayer(io, recvMasked, sendMasked, cfg),
      _state(eTranslayerState::SendHttpHandshake),
      _handshake()
{
}

void WsClientTrans::reset()
{
    _state = eTranslayerState::SendHttpHandshake;
    _handshake.reset(new WsClientHandshake(*this));
    _pktList.clear();
    _wsDecoder.reset(new WsDecoder(*this));
    _sendVec.clear();
    _needSendLen = 0;
    _sentLen = 0;
    _recvVec.clear();
    _rcvdLen = 0;    
    _wsEncoder.reset(new WsEncoder(*this));
    _canSend = true;
}

eIoAction WsClientTrans::work(eIoAction evt)
{
    eCodes code = eCodes::ST_Init;

    switch (_state)
    {
        case eTranslayerState::SendHttpHandshake:
        {
            if (evt != eIoAction::Write)
            {
                PARROT_ASSERT(false);
            }

            if (!_handshake)
            {
                _handshake.reset(new WsClientHandshake(*this));
            }

            code = _handshake->work();
            if (code != eCodes::ST_Ok)
            {
                return eIoAction::Remove;
            }

            code = sendData();
            if (code == eCodes::ST_Complete)
            {
                _state = eTranslayerState::RecvHttpHandshake;
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
        break;
        
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

            code = _handshake->work();
            if (code == eCodes::ST_NeedRecv)
            {
                return eIoAction::Read;
            }
            else if (code == eCodes::ST_Complete)
            {
                code = _handshake->getResult();
                if (code != eCodes::HTTP_Ok)
                {
                    return eIoAction::Remove;
                }

                _state = eTranslayerState::WsConnected;
                return work(eIoAction::Write);
            }
            else
            {
                PARROT_ASSERT(false);
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
