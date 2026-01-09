#include "commands/LlenCommand.hpp"
#include "AppContext.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"
#include "redis_store/values/ListValue.hpp"

std::vector<RespValue>
LlenCommand::doExecute(const std::vector<RespValue> &args,
                       AppContext &context) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));
  auto val = context.getRedisStore().get(store_key);
  if (!val) {
    result.emplace_back(RespInt(0));
    return result;
  }
  auto *list_val = std::get_if<ListValue>(&val.value());
  if (list_val == nullptr) {
    result.emplace_back(RespInt(0));
    return result;
  }
  result.emplace_back(RespInt(static_cast<int64_t>(list_val->size())));
  return result;
}

std::vector<RespValue>
LlenCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ClientConnection &connection) {
  return doExecute(args, connection.getContext());
}

std::vector<RespValue>
LlenCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ServerConnection &connection) {
  return doExecute(args, connection.getContext());
}
