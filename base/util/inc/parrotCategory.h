#ifndef __BASE_UTIL_INC_PARROTCATEGORY_H__
#define __BASE_UTIL_INC_PARROTCATEGORY_H__

#include <system_error>
#include <string>

namespace parrot {
class ParrotCategoryImpl : public std::error_category {
  public:
    const char *name() const noexcept override;
    std::string message(int ec) const noexcept override;
};

const std::error_category &ParrotCategory() noexcept;
}

#endif
