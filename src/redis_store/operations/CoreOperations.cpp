#include "redis_store/RedisStore.hpp"
#include "redis_store/RedisValue.hpp"
#include <algorithm>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

std::optional<RedisValue> RedisStore::get(const std::string &key) const {
  auto store = readStore();
  auto itr = store->find(key);
  if (itr == store->end()) {
    return std::nullopt;
  }

  const auto &val = itr->second;
  if (hasExpired(val)) {
    return std::nullopt;
  }

  return cloneRedisValue(val);
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
