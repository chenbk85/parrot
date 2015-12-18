#ifndef __COMPONENT_WEBSOCKET_INC_WSCLIENTCONN_H__
#define __COMPONENT_WEBSOCKET_INC_WSCLIENTCONN_H__

#include <memory>
#include <list>
#include <vector>
#include <ctime>
#include <string>
#include <ctime>
#include <iostream>
#include <functional>

#include "codes.h"

#include "tcpClientConn.h"
#include "timeoutGuard.h"

#include "doubleLinkedListNode.h"
#include "urlParser.h"
#include "mtRandom.h"
#include "logger.h"
#include "json.h"
#include "macroFuncs.h"
#include "ipHelper.h"


#include "wsConfig.h"
#include "wsClientTrans.h"
#include "wsTranslayer.h"
#include "wsPacketHandler.h"
#include "wsPacket.h"
#include "wsDefinition.h"

namespace parrot
{
template <class Sess>
class WsClientConn : public TcpClientConn,
                     public TimeoutGuard,
                     public DoubleLinkedListNode<WsClientConn<Sess>>
{
    using PacketHandler = WsPacketHandler<Sess, WsClientConn<Sess>>;

    enum class eWsConnState
    {
        Disconnected,
        WaitingToConnect,
        Connecting,
        Connected
    };

    enum class eWsState
    {
        NotOpened,
        Opened,
        Closing
    };

  public:
    WsClientConn(const std::string& wsUrl,
                 const WsConfig& cfg,
                 bool sendMasked = true);
    virtual ~WsClientConn() = default;
    WsClientConn(const WsClientConn&) = delete;
    WsClientConn& operator=(const WsClientConn&) = delete;

  public:
    void setPacketHandler(PacketHandler* hdr);
    void updateSession(std::shared_ptr<const Sess>&);
    void sendPacket(std::unique_ptr<WsPacket>& pkt);
    void sendPacket(std::list<std::unique_ptr<WsPacket>>& pkt);
    eIoAction handleIoEvent() override;
    // Up layer close the socket, we should sent the close packet and
    // disconnect the connection.
    void closeWebSocket(std::unique_ptr<WsPacket>& pkt);
    void setRandom(MtRandom* random);
    bool canSwitchToSend() const;
    bool canConnect() const;

  private:
    void onError(eCodes c);
    void onPacket(std::unique_ptr<WsPacket>&& pkt);
    void onConnected();
    void getNextConnectTime();
    void onOpen();
    void onPing();
    void onPong();
    void onClose(std::unique_ptr<WsPacket>&& pkt);
    void onData(std::unique_ptr<WsPacket>&& pkt);
    bool canClose();
    void doClose();
    void onDisconnect();

  private:
    eWsConnState _connState;
    eWsState _state;
    PacketHandler* _pktHandler;
    std::shared_ptr<Sess> _session;
    std::unique_ptr<WsClientTrans> _translayer;
    bool _sentClose;
    uint32_t _retryTimes;
    std::time_t _nextConnectTime;
    std::unique_ptr<UrlInfo> _urlInfo;
};

template <class Sess>
WsClientConn<Sess>::WsClientConn(const std::string& wsUrl,
                                 const WsConfig& cfg,
                                 bool sendMasked)
    : TcpClientConn(),
      TimeoutGuard(),
      DoubleLinkedListNode<WsClientConn>(),
      _state(eWsState::NotOpened),
      _pktHandler(nullptr),
      _session(new Sess()),
      _translayer(new WsClientTrans(*this, false, sendMasked, cfg)),
      _sentClose(false),
      _retryTimes(0),
      _nextConnectTime(0),
      _urlInfo()
{
    using namespace std::placeholders;

    auto onOpenCb = std::bind(&WsClientConn<Sess>::onOpen, this);
    auto onPktCb  = std::bind(&WsClientConn<Sess>::onPacket, this, _1);
    auto onErrCb  = std::bind(&WsClientConn<Sess>::onError, this, _1);

    _translayer->registerOnOpenCb(std::move(onOpenCb));
    _translayer->registerOnPacketCb(std::move(onPktCb));
    _translayer->registerOnErrorCb(std::move(onErrCb));

    _urlInfo = std::move(UrlParser::parse(wsUrl));
    if (!_urlInfo || _urlInfo->_host.empty())
    {
        PARROT_ASSERT(false);
    }

    std::list<std::string> ipv4List;
    std::list<std::string> ipv6List;

    IPHelper::getIPAddress(_urlInfo->_host, ipv4List, ipv6List);

    if (ipv4List.empty() && ipv6List.empty())
    {
        PARROT_ASSERT(false);
    }

    setRemotePort(_urlInfo->_port);
    if (!ipv6List.empty())
    {
        setRemoteAddr(*ipv6List.begin());
    }
    else
    {
        setRemoteAddr(*ipv4List.begin());
    }

    setUrlInfo(_urlInfo.get());
}

template <class Sess>
void WsClientConn<Sess>::setPacketHandler(PacketHandler* hdr)
{
    _pktHandler = hdr;
}

template <class Sess> void WsClientConn<Sess>::setRandom(MtRandom* r)
{
    _translayer->setRandom(r);
}

template <class Sess> bool WsClientConn<Sess>::canSwitchToSend() const
{
    return _translayer->canSwitchToSend();
}

template <class Sess>
void WsClientConn<Sess>::updateSession(std::shared_ptr<const Sess>& s)
{
    *_session = *s;
}

template <class Sess> void WsClientConn<Sess>::onOpen()
{
    PARROT_ASSERT(_state == eWsState::NotOpened);
    _state = eWsState::Opened;
}

template <class Sess> void WsClientConn<Sess>::onPing()
{
    std::unique_ptr<WsPacket> pkt(new WsPacket);
    pkt->setOpCode(eOpCode::Pong);
    sendPacket(pkt);
}

template <class Sess> void WsClientConn<Sess>::onPong()
{
}

template <class Sess>
void WsClientConn<Sess>::onData(std::unique_ptr<WsPacket>&& pkt)
{
    _pktHandler->onPacket(_session, std::move(pkt));
}

template <class Sess> void WsClientConn<Sess>::doClose()
{
}

template <class Sess>
void WsClientConn<Sess>::onClose(std::unique_ptr<WsPacket>&& p)
{
    _state = eWsState::Closing;
    p->decode();

    LOG_DEBUG("WsClientConn<Sess>::onClose: Close code is "
              << p->getCloseCode() << ". Reason is " << p->getCloseReason()
              << ".");

    if (_sentClose && _translayer->isAllSent())
    {
        doClose();
        _pktHandler->onClose(this, std::move(p));
    }
    else
    {
        // Closing hand wave.
        std::unique_ptr<WsPacket> pkt(new WsPacket);
        pkt->setClose(eCodes::WS_NormalClosure);
        sendPacket(pkt);
        _sentClose = true;
    }
}

template <class Sess>
void WsClientConn<Sess>::onPacket(std::unique_ptr<WsPacket>&& pkt)
{
    switch (pkt->getOpCode())
    {
        case eOpCode::Binary:
        {
            onData(std::move(pkt));
        }
        break;

        case eOpCode::Ping:
        {
            onPing();
        }
        break;

        case eOpCode::Pong:
        {
            onPong();
        }
        break;

        case eOpCode::Close:
        {
            onClose(std::move(pkt));
        }
        break;

        default:
        {
            PARROT_ASSERT(false);
        }
        break;
    }
}

template <class Sess> void WsClientConn<Sess>::onError(eCodes code)
{
    // WS_ProtocolError
    // WS_Unsupporteddata
    // WS_InvalidFramePayloadData
    // WS_PolicyViolation
    // WS_MessageTooBig
    // WS_InternalError

    std::unique_ptr<WsPacket> pkt(new WsPacket());
    pkt->setClose(code);
    sendPacket(pkt);
    _sentClose = true;
    _state     = eWsState::Closing;
}

template <class Sess>
void WsClientConn<Sess>::sendPacket(std::unique_ptr<WsPacket>& pkt)
{
    if (_state == eWsState::NotOpened)
    {
        LOG_WARN(
            "WsClientConn<Sess>::sendPacket: Websocket has not opened yet. "
            "Packet will not be sent.");
        return;
    }

    if (_state == eWsState::Closing)
    {
        LOG_WARN(
            "WsClientConn<Sess>::sendPacket: Websocket is closing. Packet will "
            "not be sent.");
        return;
    }
    _translayer->sendPacket(pkt);
}

template <class Sess>
void WsClientConn<Sess>::sendPacket(
    std::list<std::unique_ptr<WsPacket>>& pktList)
{
    if (_state == eWsState::NotOpened)
    {
        LOG_WARN(
            "WsClientConn<Sess>::sendPacket: Websocket has not opened yet. "
            "Packet will not be sent.");
        return;
    }

    if (_state == eWsState::Closing)
    {
        LOG_WARN(
            "WsClientConn<Sess>::sendPacket: Websocket is closing. Packet list "
            "will not be sent.");
        return;
    }
    _translayer->sendPacket(pktList);
}

template <class Sess> bool WsClientConn<Sess>::canClose()
{
    if (_state != eWsState::Closing)
    {
        return false;
    }

    if (_sentClose && _translayer->isAllSent())
    {
        // Closing hand wave finished.
        doClose();
        return true;
    }

    return false;
}

template <class Sess> void WsClientConn<Sess>::onConnected()
{
    _connState  = eWsConnState::Connected;
    _retryTimes = 0;
    _nextConnectTime = 0;
    _state = eWsState::NotOpened;
    setConnected();
}

template <class Sess> void WsClientConn<Sess>::getNextConnectTime()
{
    uint8_t sec      = _retryTimes >= 5 ? 5 : _retryTimes; // Max 5 secs.
    _nextConnectTime = std::time(nullptr) + sec;
}

template <class Sess> bool WsClientConn<Sess>::canConnect()
{
    if (std::time(nullptr) >= _nextConnectTime)
    {
        return true;
    }

    return false;
}

template <class Sess> eIoAction WsClientConn<Sess>::handleIoEvent()
{
    while (true)
    {
        switch (_connState)
        {
            case eWsConnState::Disconnected:
            {
                connect();

                if (isConnected())
                {
                    // Ha, connected to server so quickly.
                    onConnected();
                    continue;
                }
                else
                {
                    // Wait to connect to server.
                    _connState = eWsConnState::Connecting;
                    return eIoAction::Write;
                }
            }
            break;

            case eWsConnState::WaitingToConnect:
            {
                if (std::time(nullptr) < _nextConnectTime)
                {
                    // Not ready to reconnect.
                    return eIoAction::None;
                }

                _connState = eWsConnState::Disconnected;
                continue;
            }
            break;

            case eWsConnState::Connecting:
            {
                if (isError() || isEof())
                {
                    IoEvent::close();
                    ++_retryTimes;
                    getNextConnectTime();
                    return eIoAction::Remove;
                }

                if (isWriteAvail())
                {
                    onConnected();
                    continue;
                }
                else
                {
                    // We should never be here.
                    PARROT_ASSERT(false);
                }
            }
            break;

            case eWsConnState::Connected:
            {
                if (isError() || isEof())
                {
                    IoEvent::close();
                    _connState = eWsConnState::Disconnected;
                    continue;
                }

                eIoAction act;

                if (isReadAvail())
                {
                    act = _translayer->work(eIoAction::Read);
                    if (canClose())
                    {
                        onDisconnect();
                        continue;
                    }
                    return act;
                }

                if (isWriteAvail())
                {
                    act = _translayer->work(eIoAction::Write);
                    if (canClose())
                    {
                        onDisconnect();
                        continue;
                    }
                    return act;
                }

                PARROT_ASSERT(false);
                return eIoAction::None;
            }
            break;

            default:
            {
                PARROT_ASSERT(false);
            }
            break;
        }
    }
}

template <class Sess> void WsClientConn<Sess>::onDisconnect()
{
    IoEvent::close();
    _translayer->reset();
    _connState = eWsConnState::Disconnected;
}

template <class Sess>
void WsClientConn<Sess>::closeWebSocket(std::unique_ptr<WsPacket>& pkt)
{
    sendPacket(pkt);
    _state     = eWsState::Closing;
    _sentClose = true;
}
}

#endif
