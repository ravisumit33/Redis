#include "redis_store/RedisStore.hpp"
#include "redis_store/values/StringValue.hpp"
#include <memory>
#include <optional>
#include <string>

void RedisStore::setString(
    const std::string &key, const std::string &value,
    std::optional<std::chrono::system_clock::time_point> exp) {
  auto store = writeStore();
  auto val = std::make_unique<StringValue>(value, exp);
  store->insert_or_assign(key, std::move(val));
  notifyBlockingClients(key);
}
