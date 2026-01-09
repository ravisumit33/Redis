#include "commands/LrangeCommand.hpp"
#include "AppContext.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"
#include "redis_store/values/ListValue.hpp"

std::vector<RespValue>
LrangeCommand::doExecute(const std::vector<RespValue> &args,
                         AppContext &context) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));
  int start_idx = std::stoi(getStringValue(args.at(1)));
  int end_idx = std::stoi(getStringValue(args.at(2)));
  auto val = context.getRedisStore().get(store_key);
  RespArray resp_array;
  if (val) {
    auto *list_val = std::get_if<ListValue>(&val.value());
    if (list_val != nullptr) {
      auto list_elements = list_val->getElementsInRange(start_idx, end_idx);
      for (const auto &list_el : list_elements) {
        resp_array.add(RespBulkString(list_el));
      }
    }
  }
  result.emplace_back(std::move(resp_array));
  return result;
}

std::vector<RespValue>
LrangeCommand::executeOnImpl(const std::vector<RespValue> &args,
                             ClientConnection &connection) {
  return doExecute(args, connection.getContext());
}

std::vector<RespValue>
LrangeCommand::executeOnImpl(const std::vector<RespValue> &args,
                             ServerConnection &connection) {
  return doExecute(args, connection.getContext());
}
