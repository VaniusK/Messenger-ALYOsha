#ifndef ORM_UTILS_HPP_
#define ORM_UTILS_HPP_

#include <drogon/orm/Field.h>
#include <optional>

namespace messenger::utils {

template <typename T>
std::optional<T> fromNullable(const drogon::orm::Field &field) {
    if (!field.isNull()) {
        return field.as<T>();
    }
    return std::nullopt;
}

}  // namespace messenger::utils

#endif  // ORM_UTILS_HPP_