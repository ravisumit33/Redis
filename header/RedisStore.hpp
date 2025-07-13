#pragma once

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

class RedisStoreValue {
public:
  RedisStoreValue(const std::string &val,
                  std::optional<std::chrono::steady_clock::time_point> exp)
      : mValue(val), mExpiry(exp) {}

  bool hasExpired() const {
    if (!mExpiry) {
      return false;
    }
    return std::chrono::steady_clock::now() >= mExpiry.value();
  }

  std::string getValue() const { return mValue; }

private:
  std::string mValue;
  std::optional<std::chrono::steady_clock::time_point> mExpiry;
};

class RedisStore {
public:
  void set(const std::string &key, const std::string &value,
           std::optional<std::chrono::milliseconds> exp = std::nullopt);
  std::optional<std::string> get(const std::string &key) const;

  static RedisStore &instance() {
    static RedisStore instance;
    return instance;
  }

private:
  RedisStore() = default;
  std::unordered_map<std::string, RedisStoreValue> mStore;
  mutable std::mutex mMutex;
};
