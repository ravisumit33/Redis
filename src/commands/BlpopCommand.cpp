#include "commands/BlpopCommand.hpp"
#include "AppContext.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"

std::vector<RespValue>
BlpopCommand::doExecute(const std::vector<RespValue> &args,
                        AppContext &context) {
  std::vector<RespValue> result;
  auto store_key = getStringValue(args.at(0));
  auto timeout_s = std::stod(getStringValue(args.at(1)));
  auto [timed_out, popped_elements] =
      context.getRedisStore().removeListElementsAtBegin(store_key, 1,
                                                        timeout_s);
  if (timed_out) {
    result.emplace_back(RespArray::null());
    return result;
  }
  RespArray resp_array;
  resp_array.add(RespBulkString(store_key))
      .add(RespBulkString(popped_elements.at(0)));

  result.emplace_back(std::move(resp_array));
  return result;
}

std::vector<RespValue>
BlpopCommand::executeOnImpl(const std::vector<RespValue> &args,
                            ClientConnection &connection) {
  return doExecute(args, connection.getContext());
}

std::vector<RespValue>
BlpopCommand::executeOnImpl(const std::vector<RespValue> &args,
                            ServerConnection &connection) {
  return doExecute(args, connection.getContext());
}
