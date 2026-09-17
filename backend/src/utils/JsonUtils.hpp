#pragma once

#include <glaze/glaze.hpp>
#include <expected>
#include <string>

namespace CAM::Utils {

template <typename T>
std::expected<T, glz::error_ctx> parse_json(const std::string& json_str) {
    T obj;
    glz::error_ctx errc = glz::read_json(obj, json_str);
    if (errc) {
        return std::unexpected(errc);
    }
    return obj;
}

template <typename T>
std::expected<std::string, glz::error_ctx> serialize_json(const T& obj) {
    std::string json_str;
    glz::error_ctx errc = glz::write_json(obj, json_str);
    if (errc) {
        return std::unexpected(errc);
    }
    return json_str;
}

} // namespace CAM::Utils