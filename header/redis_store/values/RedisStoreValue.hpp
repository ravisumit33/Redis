#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

class RedisStoreValue {
public:
  enum Type { STRING, STREAM, LIST, SET };

  virtual ~RedisStoreValue() = default;

  RedisStoreValue(
      Type t,
      std::optional<std::chrono::system_clock::time_point> exp = std::nullopt)
      : m_type(t), mExpiry(exp) {}

  bool hasExpired() const {
    if (!mExpiry) {
      return false;
    }
    return std::chrono::system_clock::now() >= mExpiry.value();
  }

  virtual std::unique_ptr<RedisStoreValue> clone() const = 0;

  Type getType() const { return m_type; }

  std::string getTypeStr() const {
    static std::unordered_map<Type, std::string> type_str = {
        {STRING, "string"}, {STREAM, "stream"}, {LIST, "list"}, {SET, "set"}};
    auto it = type_str.find(m_type);
    if (it == type_str.end()) {
      throw std::logic_error("Unknown type of redis store value");
    }
    return it->second;
  }

private:
  Type m_type;
  std::optional<std::chrono::system_clock::time_point> mExpiry;
};
