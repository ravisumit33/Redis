#include "commands/IncrCommand.hpp"
#include "redis_store/RedisStore.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"
#include "redis_store/values/StringValue.hpp"
#include <string>

std::vector<RespValue>
IncrCommand::doExecute(const std::vector<RespValue> &args,
                       RedisStore &store) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));

  auto store_val_ptr = store.get(store_key);
  if (!store_val_ptr) {
    store.setString(store_key, "1");
    result.emplace_back(RespInt(1));
    return result;
  }

  auto *string_val = std::get_if<StringValue>(&store_val_ptr.value());
  if (string_val == nullptr) {
    result.emplace_back(RespError("value is not an integer or out of range"));
    return result;
  }

  int64_t int_val{};
  try {
    int_val = std::stoll(string_val->getValue());
  } catch (...) {
    result.emplace_back(RespError("value is not an integer or out of range"));
    return result;
  }

  ++int_val;
  store.setString(store_key, std::to_string(int_val));
  result.emplace_back(RespInt(int_val));
  return result;
}

std::vector<RespValue>
IncrCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ClientConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}

std::vector<RespValue>
IncrCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ServerConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}
