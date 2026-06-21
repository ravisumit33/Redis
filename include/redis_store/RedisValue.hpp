#pragma once

#include "redis_store/values/ListValue.hpp"
#include "redis_store/values/SetValue.hpp"
#include "redis_store/values/StreamValue.hpp"
#include "redis_store/values/StringValue.hpp"
#include "utils/genericUtils.hpp"
#include <cstdint>
#include <string>
#include <variant>

using RedisValue = std::variant<StringValue, ListValue, SetValue, StreamValue>;

enum class RedisValueType : std::uint8_t { STRING, LIST, SET, STREAM };

inline RedisValueType getRedisValueType(const RedisValue &value) {
  return static_cast<RedisValueType>(value.index());
}

inline std::string getRedisValueTypeStr(const RedisValue &value) {
  return std::visit(
      [](const auto &val) constexpr -> std::string {
        using T = std::decay_t<decltype(val)>;

        if constexpr (std::is_same_v<T, StringValue>) {
          return "string";
        } else if constexpr (std::is_same_v<T, ListValue>) {
          return "list";
        } else if constexpr (std::is_same_v<T, SetValue>) {
          return "set";
        } else if constexpr (std::is_same_v<T, StreamValue>) {
          return "stream";
        } else {
          static_assert(always_false<T>, "Unsupported Redis value type");
        }
      },
      value);
}

inline bool hasExpired(const RedisValue &value) {
  return std::visit(
      [](const auto &val) { return val.getExpiryInfo().hasExpired(); }, value);
}

inline RedisValue cloneRedisValue(const RedisValue &value) { return value; }