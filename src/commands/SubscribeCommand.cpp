#include "commands/SubscribeCommand.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include <memory>
#include <string>

CommandRegistrar<SubscribeCommand> SubscribeCommand::registrar("SUBSCRIBE");

std::vector<std::unique_ptr<RespType>> SubscribeCommand::executeImpl(
    const std::vector<std::unique_ptr<RespType>> &args,
    Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto channel_name = static_cast<RespBulkString &>(*args.at(0)).getValue();
  auto &client_connection = static_cast<ClientConnection &>(connection);
  std::size_t subscription_count =
      client_connection.subscribeToChannel(channel_name);
  auto resp_array = std::make_unique<RespArray>();
  resp_array->add(std::make_unique<RespBulkString>("subscribe"))
      ->add(std::make_unique<RespBulkString>(channel_name))
      ->add(std::make_unique<RespInt>(subscription_count));
  result.push_back(std::move(resp_array));
  return result;
}
