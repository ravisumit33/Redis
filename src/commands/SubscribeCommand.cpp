#include "commands/SubscribeCommand.hpp"
#include "RespType.hpp"
#include "connections/Subscriptions.hpp"
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

std::vector<RespValue>
SubscribeCommand::doExecute(const std::vector<RespValue> &args,
                            Subscriptions &subs,
                            RedisChannelManager &channels) {
  std::vector<RespValue> result;
  auto channel_name = getStringValue(args.at(0));
  const std::size_t subscription_count = subs.subscribe(channels, channel_name);
  RespArray resp_array;
  resp_array.add(RespBulkString("subscribe"))
      .add(RespBulkString(channel_name))
      .add(RespInt(static_cast<int64_t>(subscription_count)));
  result.emplace_back(std::move(resp_array));
  return result;
}
