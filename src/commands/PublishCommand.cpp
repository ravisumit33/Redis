#include "commands/PublishCommand.hpp"
#include "RedisChannel.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"

std::vector<RespValue>
PublishCommand::doExecute(const std::vector<RespValue> &args,
                          RedisChannelManager &channels) {
  std::vector<RespValue> result;
  auto channel_name = getStringValue(args.at(0));
  auto message = getStringValue(args.at(1));
  auto *channel = channels.getChannel(channel_name);
  std::size_t subscription_count = channel->publish(message);
  result.emplace_back(RespInt(static_cast<int64_t>(subscription_count)));
  return result;
}

std::vector<RespValue>
PublishCommand::executeOnImpl(const std::vector<RespValue> &args,
                              ClientConnection &connection) {
  return doExecute(args, connection.getContext().getChannelManager());
}

std::vector<RespValue>
PublishCommand::executeOnImpl(const std::vector<RespValue> &args,
                              ServerConnection &connection) {
  return doExecute(args, connection.getContext().getChannelManager());
}
