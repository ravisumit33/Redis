#include "commands/PublishCommand.hpp"
#include "RedisChannel.hpp"
#include "RespType.hpp"
#include <memory>
#include <string>

CommandRegistrar<PublishCommand> PublishCommand::registrar("PUBLISH");

std::vector<std::unique_ptr<RespType>>
PublishCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                            Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto channel_name = static_cast<RespBulkString &>(*args.at(0)).getValue();
  auto message = static_cast<RespBulkString &>(*args.at(1)).getValue();
  auto channel = RedisChannelManager::instance().getChannel(channel_name);
  std::size_t subscription_count = channel->publish(message);
  result.push_back(std::make_unique<RespInt>(subscription_count));
  return result;
}
