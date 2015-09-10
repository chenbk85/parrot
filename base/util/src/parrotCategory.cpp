#include "parrotCategory.h"
#include "codes.h"

namespace parrot
{
    const char * ParrotCategoryImpl::name() const noexcept
    {
        return "parrot";
    }

    std::string ParrotCategoryImpl::message(int ec) const noexcept
    {
        std::string ret;
        switch (static_cast<Codes>(ec))
        {
            /////////////////////////////////////////////////
            /// Http
            ////////////////
            case Codes::HTTP_Continue:
                ret = "HTTP_Continue";
                break;
            case Codes::HTTP_SwitchingProtocols:
                ret = "HTTP_SwitchingProtocols";
                break;
            case Codes::HTTP_Processing:
                ret = "HTTP_Processing";
                break;
            case Codes::HTTP_Ok:
                ret = "HTTP_Ok";
                break;
            case Codes::HTTP_BadRequest:
                ret = "HTTP_BadRequest";
                break;
            case Codes::HTTP_UpgradeRequired:
                ret = "HTTP_UpgradeRequired";
                break;

                /////////////////////////////////////////////////
                /// Websockets
                ////////////////
            case Codes::WS_NormalClosure:
                ret = "WS_NormalClosure";
                break;
            case Codes::WS_GoingAway:
                ret = "WS_GoingAway";
                break;
            case Codes::WS_ProtocolError:
                ret = "WS_ProtocolError";
                break;
            case Codes::WS_UnsupportedData:
                ret = "WS_UnsupportedData";
                break;
            case Codes::WS_NoStatusRcvd:
                ret = "WS_NoStatusRcvd";
                break;
            case Codes::WS_AbnormalClosure:
                ret = "WS_AbnormalClosure";
                break;
            case Codes::WS_InvalidFramePayloadData:
                ret = "WS_InvalidFramePayloadData";
                break;
            case Codes::WS_PolicyViolation:
                ret = "WS_PolicyViolation";
                break;
            case Codes::WS_MessageTooBig:
                ret = "WS_MessageTooBig";
                break;
            case Codes::WS_MandatoryExt:
                ret = "WS_MandatoryExt";
                break;
            case Codes::WS_InternalError:
                ret = "WS_InternalError";
                break;
            case Codes::WS_TlsHandshake:
                ret = "WS_TlsHandshake";
                break;
        
                /////////////////////////////////////////////////
                /// Status codes for our system.
                ////////////////
            case Codes::ST_Init:
                ret = "ST_Init";
                break;
            case Codes::ST_Connecting:
                ret = "ST_Connecting";
                break;
            case Codes::ST_Connected:
                ret = "ST_Connected";
                break;
            case Codes::ST_Sending:
                ret = "ST_Sending";
                break;
            case Codes::ST_Sent:
                ret = "ST_Sent";
                break;
            case Codes::ST_NeedSend:
                ret = "ST_NeedSend";
                break;
            case Codes::ST_Recving:
                ret = "ST_Recving";
                break;
            case Codes::ST_Received:
                ret = "ST_Received";
                break;
            case Codes::ST_NeedRecv:
                ret = "ST_NeedRecv";
                break;
            case Codes::ST_RetryLater:
                ret = "ST_RetryLater";
                break;
            case Codes::ST_RetryWhenReadable:
                ret = "ST_RetryWhenReadable";
                break;
            case Codes::ST_RetryWhenWriteable:
                ret = "ST_RetryWhenWriteable";
                break;
            case Codes::ST_Ok:
                ret = "ST_Ok";
                break;

                /////////////////////////////////////////////////
                /// Error codes.
                ////////////////
            case Codes::ERR_Fail:
                ret = "ERR_Fail";
                break;
            case Codes::ERR_FileOpen:
                ret = "ERR_FileOpen";
                break;
            case Codes::ERR_FileWrite:
                ret = "ERR_FileWrite";
                break;
            case Codes::ERR_FileRead:
                ret = "ERR_FileRead";
                break;
            case Codes::ERR_Send:
                ret = "ERR_Send";
                break;
            case Codes::ERR_Recv:
                ret = "ERR_Recv";
                break;
            case Codes::ERR_HttpHeader:
                ret = "ERR_HttpHeader";
                break;
            default:
                return std::string("Unknown code ") + std::to_string(ec);
        }
        return ret;
    }

    const std::error_category& ParrotCategory() noexcept
    {
        // C++11 guarantees that this initialisation is thread-safe.
        static ParrotCategoryImpl obj;
        return obj;
    }
}
