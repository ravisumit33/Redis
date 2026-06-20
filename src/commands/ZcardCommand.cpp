#include "commands/ZcardCommand.hpp"
#include "redis_store/RedisStore.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"
#include "redis_store/values/SetValue.hpp"

std::vector<RespValue>
ZcardCommand::doExecute(const std::vector<RespValue> &args,
                        RedisStore &store) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));

  std::size_t set_size = 0;
  auto val = store.get(store_key);
  if (val) {
    auto *set_val = std::get_if<SetValue>(&val.value());
    if (set_val != nullptr) {
      set_size = set_val->size();
    }
  }
  result.emplace_back(RespInt(static_cast<int64_t>(set_size)));
  return result;
}

std::vector<RespValue>
ZcardCommand::executeOnImpl(const std::vector<RespValue> &args,
                            ClientConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}

std::vector<RespValue>
ZcardCommand::executeOnImpl(const std::vector<RespValue> &args,
                            ServerConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}
