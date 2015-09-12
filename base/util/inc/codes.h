#ifndef __BASE_UTIL_INC_CODES_H__
#define __BASE_UTIL_INC_CODES_H__

#include <system_error>
#include "parrotCategory.h"

namespace parrot 
{
    enum class eCodes
    {
        // 0 - 999 status codes are for HTTP standand codes.
        HTTP_Continue                    = 100,
        HTTP_SwitchingProtocols          = 101,
        HTTP_Processing                  = 102,
        HTTP_Ok                          = 200,
        HTTP_BadRequest                  = 400,
        HTTP_PayloadTooLarge             = 413,
        HTTP_UpgradeRequired             = 426,

        // According to RFC6455, websocket defines status codes between 1000
        // and 4999.
        WS_NormalClosure                 = 1000,
        WS_GoingAway                     = 1001,
        WS_ProtocolError                 = 1002,
        WS_UnsupportedData               = 1003,
        // 1004 is reserved.
        WS_NoStatusRcvd                  = 1005,
        WS_AbnormalClosure               = 1006,
        WS_InvalidFramePayloadData       = 1007,
        WS_PolicyViolation               = 1008,
        WS_MessageTooBig                 = 1009,
        WS_MandatoryExt                  = 1010,
        WS_InternalError                 = 1011,
        WS_TlsHandshake                  = 1015,
        
        // Parrot status codes start from 100,000.
        ST_Init                          = 100000,
        ST_Connecting                    = 100001,
        ST_Connected                     = 100002,
        ST_Sending                       = 100010,
        ST_Sent                          = 100011,
        ST_NeedSend                      = 100012,
        ST_Recving                       = 100020,
        ST_Received                      = 100021,
        ST_NeedRecv                      = 100022,
        ST_RetryLater                    = 100030,
        ST_RetryWhenReadable             = 100031,
        ST_RetryWhenWriteable            = 100032,
        ST_WSHttpHandshakeOk             = 100033,
        ST_Complete                      = 100100,

        ST_Ok                            = 1000000,

        // Common error codes start from 1,000,000.
        ERR_Fail                         = 5000000,
        ERR_FileOpen                     = 5000001,
        ERR_FileWrite                    = 5000002,
        ERR_FileRead                     = 5000003,
        ERR_Send                         = 5000004,
        ERR_Recv                         = 5000005,
        ERR_HttpHeader                   = 5000006
    };

    std::error_code make_error_code(eCodes e) noexcept;

    std::error_condition make_error_condition(eCodes e) noexcept;
}

namespace std
{
    template <>
        struct is_error_code_enum<parrot::eCodes> : public true_type {};
}

#endif
