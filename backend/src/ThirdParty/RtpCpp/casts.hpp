#pragma once

#include <cassert>
#include <concepts>
#include <utility>

namespace RtpCpp::Detail {

template <std::integral To, std::integral From>
[[nodiscard]] constexpr To narrow_cast(From value) noexcept {
    assert(std::in_range<To>(value) && "narrow_cast failed: value out of bounds for target type");
    return static_cast<To>(value);
}

} // namespace RtpCpp


