#pragma once

#include <source_location>
#include <span>
#include <string_view>

template <typename T, typename... Args>
concept Logger = requires(T t, const T::Type logLevel, const size_t reservedMsgSize, const T::Type logType,
                          const std::source_location &location, const std::string_view format, const Args... args,
                          const std::span<const std::byte> buffer) {
    typename T::Type;
    { T::init(logLevel, reservedMsgSize) } -> std::same_as<void>;
    { T::clear() } -> std::same_as<void>;
    { T::log(logType, location, format, args...) } -> std::same_as<void>;
    { T::log(buffer) } -> std::same_as<void>;
};
