#include "commands/LpopCommand.hpp"
#include "redis_store/RedisStore.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"

std::vector<RespValue>
LpopCommand::doExecute(const std::vector<RespValue> &args,
                       RedisStore &store) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));
  unsigned el_count = 1;
  if (args.size() == 2) {
    el_count = std::stoul(getStringValue(args.at(1)));
  }
  auto [removed, popped_elements] =
      store.removeListElementsAtBegin(store_key, el_count);
  if (popped_elements.empty()) {
    result.emplace_back(RespBulkString());
    return result;
  }
  if (el_count == 1) {
    result.emplace_back(RespBulkString(popped_elements.at(0)));
    return result;
  }

  RespArray resp_array;
  for (const auto &elem : popped_elements) {
    resp_array.add(RespBulkString(elem));
  }
  result.emplace_back(std::move(resp_array));
  return result;
}

std::vector<RespValue>
LpopCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ClientConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}

std::vector<RespValue>
LpopCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ServerConnection &connection) {
  return doExecute(args, connection.getContext().getRedisStore());
}
