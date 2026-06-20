#include "commands/TypeCommand.hpp"
#include "redis_store/RedisStore.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"
#include "redis_store/RedisValue.hpp"

std::vector<RespValue>
TypeCommand::doExecute(const std::vector<RespValue> &args,
                       RedisStore &store) {
  std::vector<RespValue> result;
  auto arg1 = getStringValue(args.at(0));
  auto store_val_ptr = store.get(arg1);
  if (!store_val_ptr) {
    result.emplace_back(RespString("none"));
    return result;
  }
  auto val_type = getRedisValueTypeStr(store_val_ptr.value());
  result.emplace_back(RespString(val_type));
  return result;
}

std::vector<RespValue>
TypeCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ClientConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}

std::vector<RespValue>
TypeCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ServerConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}
