#include "commands/UnsubscribeCommand.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"

std::vector<RespValue>
UnsubscribeCommand::executeOnImpl(const std::vector<RespValue> &args,
                                  ClientConnection &connection) {
  std::vector<RespValue> result;
  auto channel_name = getStringValue(args.at(0));
  std::size_t subscription_count =
      connection.unsubscribeFromChannel(channel_name);
  RespArray resp_array;
  resp_array.add(RespBulkString("unsubscribe"))
      .add(RespBulkString(channel_name))
      .add(RespInt(static_cast<int64_t>(subscription_count)));
  result.emplace_back(std::move(resp_array));
  return result;
}
