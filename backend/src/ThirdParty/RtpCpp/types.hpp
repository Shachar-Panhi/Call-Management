#pragma once
#include <array>
#include <cstdint>
#include <ranges>
#include <system_error>
#include <vector>

namespace RtpCpp {

template <typename T>
concept ContiguousBuffer =
    std::ranges::contiguous_range<T> && sizeof(std::ranges::range_value_t<T>) == 1;

template <typename C>
concept ResizableContiguousBuffer =
    ContiguousBuffer<C> && requires(C& c, std::size_t n) { c.resize(n); };

using RtpBuffer = std::vector<std::uint8_t>;

template <ContiguousBuffer B>
class RtpPacket;
using StaticRtpPacket = RtpPacket<std::array<std::uint8_t, 1500>>;
using DynamicRtpPacket = RtpPacket<std::vector<std::uint8_t>>;
using RtpPacketView = RtpPacket<std::span<std::uint8_t>>;

constexpr std::size_t kMaxRtpPacketSize = 1500;

enum class SocketMode : std::uint8_t { kSingle, kDual };

enum class Result : std::uint8_t {
    kSuccess,
    kError,
    kInvalidPacket,
    kBufferTooSmall,
    kNotImplemented,
    kFixedBufferTooSmall,
    kParseBufferOverflow,
    kParseExtensionOverflow,
    kInvalidHeaderLength,
    kInvalidRtpHeader,
    kInvalidCsrcCount
};


// 2. Define the custom error category
class RtpPacketCatagory : public std::error_category {
public:
    // Unique name identifier for this category
    const char* name() const noexcept override { return "MyAppErrorCategory"; }

    // Map the integer error value to a descriptive string message
    std::string message(int ev) const override {
        switch (static_cast<Result>(ev)) {
        case Result::kSuccess:
            return "Success";
        case Result::kBufferTooSmall:
            return "Buffer too small";
        case Result::kParseBufferOverflow:
            return "Parse overflow buffer length";
        case Result::kInvalidHeaderLength:
            return "Invalid header length";
        case Result::kInvalidCsrcCount:
            return "Invalid csrc count";
        case Result::kFixedBufferTooSmall:
            return "Fixed buffer too small";
        case Result::kParseExtensionOverflow:
            return "Extension overflow buffer length";
        default:
            return "Unknown internal error.";
        }
    }
};

// 3. Singleton instance provider
inline const std::error_category& get_rtp_packet_catagory() noexcept {
    static RtpPacketCatagory instance;
    return instance;
}

// 4. ADL helper function (Must be in the SAME namespace as the enum)
inline std::error_code make_error_code(Result e) noexcept {
    return {static_cast<int>(e), get_rtp_packet_catagory()};
}


} // namespace RtpCpp


// 5. Specialize the std trait (Must be explicitly in the std namespace)
namespace std {
template <>
struct is_error_code_enum<RtpCpp::Result> : true_type {};
} // namespace std

