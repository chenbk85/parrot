#include "codes.h"

namespace parrot
{
std::error_code make_error_code(eCodes e) noexcept
{
    return std::error_code{static_cast<int>(e), ParrotCategory()};
}

std::error_condition make_error_condition(eCodes e) noexcept
{
    return std::error_condition{static_cast<int>(e), ParrotCategory()};
}
}
