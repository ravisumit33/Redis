#include "redis_store/RedisStore.hpp"
#include "redis_store/values/StringValue.hpp"
#include <optional>
#include <string>

void RedisStore::setString(
    const std::string &key, const std::string &value,
    std::optional<std::chrono::system_clock::time_point> exp) {
  auto store = writeStore();
  auto val = StringValue(value, exp);
  store->insert_or_assign(key, val);
  notifyBlockingClients(key);
}
