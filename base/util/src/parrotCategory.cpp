#include "parrotCategory.h"
#include "codes.h"

namespace parrot
{
    const char * ParrotCat::name() const noexcept
    {
        return "parrot";
    }

    std::string ParrotCat::message(int ec) const noexcept
    {
        switch (static_cast<Codes>(ec))
        {
            case Codes::OK:
                return "OK";
            default:
                return std::string("Unknown code ") + std::to_string(ec);
        }
    }

    const std::error_category& ParrotCategory() noexcept
    {
        // C++11 guarantees that this initialisation is thread-safe.
        static ParrotCat obj;
        return obj;
    }
}
