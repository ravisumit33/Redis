#include "commands/KeysCommand.hpp"
#include "redis_store/RedisStore.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"

std::vector<RespValue>
KeysCommand::doExecute(const std::vector<RespValue> &args,
                       RedisStore &store) {
  std::vector<RespValue> result;
  auto arg1 = getStringValue(args.at(0));
  if (arg1 != "*") {
    result.emplace_back(RespError("Invalid argument: " + arg1));
    return result;
  }
  auto store_keys = store.getKeys();
  RespArray resp_array;
  for (const auto &store_key : store_keys) {
    resp_array.add(RespBulkString(store_key));
  }
  result.emplace_back(std::move(resp_array));
  return result;
}

std::vector<RespValue>
KeysCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ClientConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}

std::vector<RespValue>
KeysCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ServerConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}
