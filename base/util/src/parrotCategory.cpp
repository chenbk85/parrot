#include "parrotCategory.h"
#include "codes.h"

namespace parrot {
const char *ParrotCategoryImpl::name() const noexcept {
    return "parrot";
}

std::string ParrotCategoryImpl::message(int ec) const noexcept {
    std::string ret;
    switch (static_cast<eCodes>(ec)) {
    /////////////////////////////////////////////////
    /// Http
    ////////////////
    case eCodes::HTTP_Continue:
        ret = "Continue";
        break;
    case eCodes::HTTP_SwitchingProtocols:
        ret = "Switching Protocols";
        break;
    case eCodes::HTTP_Processing:
        ret = "Processing";
        break;
    case eCodes::HTTP_Ok:
        ret = "OK";
        break;
    case eCodes::HTTP_BadRequest:
        ret = "Bad Request";
        break;
    case eCodes::HTTP_PayloadTooLarge:
        ret = "Payload Too Large";
        break;
    case eCodes::HTTP_UpgradeRequired:
        ret = "Upgrade Required";
        break;

    /////////////////////////////////////////////////
    /// Websockets
    ////////////////
    case eCodes::WS_NormalClosure:
        ret = "WS_NormalClosure";
        break;
    case eCodes::WS_GoingAway:
        ret = "WS_GoingAway";
        break;
    case eCodes::WS_ProtocolError:
        ret = "WS_ProtocolError";
        break;
    case eCodes::WS_UnsupportedData:
        ret = "WS_UnsupportedData";
        break;
    case eCodes::WS_NoStatusRcvd:
        ret = "WS_NoStatusRcvd";
        break;
    case eCodes::WS_AbnormalClosure:
        ret = "WS_AbnormalClosure";
        break;
    case eCodes::WS_InvalidFramePayloadData:
        ret = "WS_InvalidFramePayloadData";
        break;
    case eCodes::WS_PolicyViolation:
        ret = "WS_PolicyViolation";
        break;
    case eCodes::WS_MessageTooBig:
        ret = "WS_MessageTooBig";
        break;
    case eCodes::WS_MandatoryExt:
        ret = "WS_MandatoryExt";
        break;
    case eCodes::WS_InternalError:
        ret = "WS_InternalError";
        break;
    case eCodes::WS_TlsHandshake:
        ret = "WS_TlsHandshake";
        break;

    /////////////////////////////////////////////////
    /// Status codes for our system.
    ////////////////
    case eCodes::ST_Init:
        ret = "ST_Init";
        break;
    case eCodes::ST_Connecting:
        ret = "ST_Connecting";
        break;
    case eCodes::ST_Connected:
        ret = "ST_Connected";
        break;
    case eCodes::ST_Sending:
        ret = "ST_Sending";
        break;
    case eCodes::ST_Sent:
        ret = "ST_Sent";
        break;
    case eCodes::ST_NeedSend:
        ret = "ST_NeedSend";
        break;
    case eCodes::ST_Recving:
        ret = "ST_Recving";
        break;
    case eCodes::ST_Received:
        ret = "ST_Received";
        break;
    case eCodes::ST_NeedRecv:
        ret = "ST_NeedRecv";
        break;
    case eCodes::ST_RetryLater:
        ret = "ST_RetryLater";
        break;
    case eCodes::ST_RetryWhenReadable:
        ret = "ST_RetryWhenReadable";
        break;
    case eCodes::ST_RetryWhenWriteable:
        ret = "ST_RetryWhenWriteable";
        break;
    case eCodes::ST_Complete:
        ret = "ST_Complete";
        break;
    case eCodes::ST_Ok:
        ret = "ST_Ok";
        break;

    /////////////////////////////////////////////////
    /// Error codes.
    ////////////////
    case eCodes::ERR_Fail:
        ret = "ERR_Fail";
        break;
    case eCodes::ERR_FileOpen:
        ret = "ERR_FileOpen";
        break;
    case eCodes::ERR_FileWrite:
        ret = "ERR_FileWrite";
        break;
    case eCodes::ERR_FileRead:
        ret = "ERR_FileRead";
        break;
    case eCodes::ERR_Send:
        ret = "ERR_Send";
        break;
    case eCodes::ERR_Recv:
        ret = "ERR_Recv";
        break;
    case eCodes::ERR_HttpHeader:
        ret = "ERR_HttpHeader";
        break;
    default:
        return std::string("Unknown code ") + std::to_string(ec);
    }
    return ret;
}

const std::error_category &ParrotCategory() noexcept {
    // C++11 guarantees that this initialisation is thread-safe.
    static ParrotCategoryImpl obj;
    return obj;
}
}
