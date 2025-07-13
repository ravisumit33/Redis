#include "RedisStore.hpp"
#include <chrono>
#include <mutex>
#include <optional>

void RedisStore::set(const std::string &key, const std::string &value,
                     std::optional<std::chrono::milliseconds> exp) {
  std::lock_guard<std::mutex> lock(mMutex);
  std::optional<std::chrono::steady_clock::time_point> expiry = std::nullopt;
  if (exp) {
    expiry = std::chrono::steady_clock::now() + exp.value();
  }
  RedisStoreValue val(value, expiry);
  mStore.insert_or_assign(key, val);
}

std::optional<std::string> RedisStore::get(const std::string &key) const {
  std::lock_guard<std::mutex> lock(mMutex);
  auto it = mStore.find(key);
  if (it == mStore.end()) {
    return std::nullopt;
  }

  const RedisStoreValue &val = it->second;
  if (val.hasExpired()) {
    return std::nullopt;
  }

  return val.getValue();
}
