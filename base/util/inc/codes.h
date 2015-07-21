#ifndef __BASE_UTIL_INC_CODES_H__
#define __BASE_UTIL_INC_CODES_H__

#include <system_error>
#include "parrotCategory.h"

namespace parrot
{
    enum class Codes
    {
        OK                   = 200
    };

    std::error_code make_error_code(Codes e) noexcept
    {
        return std::error_code{static_cast<int>(e), ParrotCategory()};
    }

    std::error_condition make_error_condition(Codes e) noexcept
    {
        return std::error_condition{static_cast<int>(e), ParrotCategory()};
    }
}

namespace std
{
    template <>
        struct is_error_code_enum<parrot::Codes> : public true_type {};
}

#endif
