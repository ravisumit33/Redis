#include "commands/PublishCommand.hpp"
#include "RedisChannel.hpp"
#include "RespType.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

std::vector<RespValue>
PublishCommand::doExecute(const std::vector<RespValue> &args,
                          RedisChannelManager &channels) {
  std::vector<RespValue> result;
  auto channel_name = getStringValue(args.at(0));
  auto message = getStringValue(args.at(1));
  auto *channel = channels.getChannel(channel_name);
  const std::size_t subscription_count = channel->publish(message);
  result.emplace_back(RespInt(static_cast<int64_t>(subscription_count)));
  return result;
}
