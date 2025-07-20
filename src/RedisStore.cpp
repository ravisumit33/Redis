#include "RedisStore.hpp"
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>

void RedisStore::setString(const std::string &key, const std::string &value,
                           std::optional<std::chrono::milliseconds> exp) {
  std::lock_guard<std::mutex> lock(mMutex);
  std::optional<std::chrono::steady_clock::time_point> expiry = std::nullopt;
  if (exp) {
    expiry = std::chrono::steady_clock::now() + exp.value();
  }
  auto val = std::make_unique<StringValue>(value, expiry);
  mStore.insert_or_assign(key, std::move(val));
}

void RedisStore::addStreamEntry(const std::string &key,
                                const StreamValue::StreamId &entry_id,
                                StreamValue::StreamEntry entry) {
  std::lock_guard<std::mutex> lock(mMutex);
  auto it = mStore.find(key);

  if (it == mStore.end()) {
    auto val = std::make_unique<StreamValue>();
    val->addEntry(entry_id, std::move(entry));
    mStore[key] = std::move(val);
  } else {
    auto &stream = static_cast<StreamValue &>(*it->second);
    stream.addEntry(entry_id, std::move(entry));
  }
}

std::optional<std::unique_ptr<RedisStoreValue>>
RedisStore::get(const std::string &key) const {
  std::lock_guard<std::mutex> lock(mMutex);
  auto it = mStore.find(key);
  if (it == mStore.end()) {
    return std::nullopt;
  }

  auto &val = it->second;
  if (val->hasExpired()) {
    return std::nullopt;
  }

  return val->clone();
}
