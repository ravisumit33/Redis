#include "commands/SubscribeCommand.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"

std::vector<RespValue>
SubscribeCommand::executeOnImpl(const std::vector<RespValue> &args,
                                ClientConnection &connection) {
  std::vector<RespValue> result;
  auto channel_name = getStringValue(args.at(0));
  std::size_t subscription_count = connection.subscribeToChannel(channel_name);
  RespArray resp_array;
  resp_array.add(RespBulkString("subscribe"))
      .add(RespBulkString(channel_name))
      .add(RespInt(static_cast<int64_t>(subscription_count)));
  result.emplace_back(std::move(resp_array));
  return result;
}
