#include "codes.h"
#include "ioEvent.h"
#include "macroFuncs.h"
#include "wsPacket.h"
#include "wsServerTrans.h"

namespace parrot
{
WsServerTrans::WsServerTrans(IoEvent& io,
                             bool recvMasked,
                             bool sendMasked,
                             const WsConfig& cfg)
    : WsTranslayer(io, recvMasked, sendMasked, cfg),
      _state(eTranslayerState::RecvHttpHandshake),
      _handshake()
{
}

eIoAction WsServerTrans::work(eIoAction evt)
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

            if (!_handshake)
            {
                _handshake.reset(new WsServerHandshake(*this));
            }

            code = _handshake->work();
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
                if (_handshake->getResult() != eCodes::HTTP_SwitchingProtocols)
                {
                    return eIoAction::Remove;
                }

                _handshake.reset(nullptr);
                _state   = WsConnected;
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
