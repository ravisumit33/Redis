#include "commands/GetCommand.hpp"
#include "redis_store/RedisStore.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"
#include "redis_store/values/StringValue.hpp"

std::vector<RespValue> GetCommand::doExecute(const std::vector<RespValue> &args,
                                             RedisStore &store) {
  std::vector<RespValue> result;

  auto key = getStringValue(args.at(0));
  auto stored_value = store.get(key);
  if (!stored_value) {
    result.emplace_back(RespBulkString());
    return result;
  }

  auto *string_val = std::get_if<StringValue>(&stored_value.value());
  if (string_val == nullptr) {
    result.emplace_back(RespError("Invalid Key"));
    return result;
  }
  result.emplace_back(RespBulkString(string_val->getValue()));
  return result;
}

std::vector<RespValue>
GetCommand::executeOnImpl(const std::vector<RespValue> &args,
                          ClientConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}

std::vector<RespValue>
GetCommand::executeOnImpl(const std::vector<RespValue> &args,
                          ServerConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}
