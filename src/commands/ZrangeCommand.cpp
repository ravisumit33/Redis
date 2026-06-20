#include "commands/ZrangeCommand.hpp"
#include "redis_store/RedisStore.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"
#include "redis_store/values/SetValue.hpp"

std::vector<RespValue>
ZrangeCommand::doExecute(const std::vector<RespValue> &args,
                         RedisStore &store) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));
  int start_idx = std::stoi(getStringValue(args.at(1)));
  int end_idx = std::stoi(getStringValue(args.at(2)));
  auto val = store.get(store_key);
  RespArray resp_array;
  if (val) {
    auto *set_val = std::get_if<SetValue>(&val.value());
    if (set_val != nullptr) {
      auto set_members = set_val->getElementsInRange(start_idx, end_idx);
      for (const auto &set_member : set_members) {
        resp_array.add(RespBulkString(set_member));
      }
    }
  }
  result.emplace_back(std::move(resp_array));
  return result;
}

std::vector<RespValue>
ZrangeCommand::executeOnImpl(const std::vector<RespValue> &args,
                             ClientConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}

std::vector<RespValue>
ZrangeCommand::executeOnImpl(const std::vector<RespValue> &args,
                             ServerConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}
