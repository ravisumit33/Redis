#include "redis_store/RedisStore.hpp"
#include "redis_store/values/RedisStoreValue.hpp"
#include <algorithm>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <vector>

std::optional<std::unique_ptr<RedisStoreValue>>
RedisStore::get(const std::string &key) const {
  auto store = readStore();
  auto it = store->find(key);
  if (it == store->end()) {
    return std::nullopt;
  }

  auto &val = it->second;
  if (val->hasExpired()) {
    return std::nullopt;
  }

  return val->clone();
}

bool RedisStore::keyExists(const std::string &key) const {
  auto store = readStore();
  return store->contains(key);
}

std::vector<std::string> RedisStore::getKeys() const {
  std::vector<std::string> keys;
  auto store = readStore();
  std::transform(store->begin(), store->end(), std::back_inserter(keys),
                 [](const auto &pair) { return pair.first; });
  return keys;
}

void RedisStore::notifyBlockingClients(const std::string &key) const {
  auto queue = getBlockingQueue(key);
  queue->notify_one();
}
