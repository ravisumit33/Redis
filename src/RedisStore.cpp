#include "RedisStore.hpp"
#include <mutex>
#include <optional>

void RedisStore::set(const std::string &key, const std::string &value) {
  std::lock_guard<std::mutex> lock(mMutex);
  mStore[key] = value;
}

std::optional<std::string> RedisStore::get(const std::string &key) const {
  std::lock_guard<std::mutex> lock(mMutex);
  auto it = mStore.find(key);
  if (it == mStore.end()) {
    return std::nullopt;
  }
  return it->second;
}
