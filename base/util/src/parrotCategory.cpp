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
        switch (static_cast<Codes>(ec))
        {
            case Codes::OK:
                return "OK";
            case Codes::ERR_FILE_OPEN:
                return "ERR_FILE_OPEN";
            case Codes::ERR_FILE_WRITE:
                return "ERR_FILE_WRITE";
            case Codes::ERR_FILE_READ:
                return "ERR_FILE_READ";
            default:
                return std::string("Unknown code ") + std::to_string(ec);
        }
    }

    const std::error_category& ParrotCategory() noexcept
    {
        // C++11 guarantees that this initialisation is thread-safe.
        static ParrotCategoryImpl obj;
        return obj;
    }
}
