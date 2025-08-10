#include "commands/MultiCommand.hpp"
#include "Connection.hpp"
#include "RespType.hpp"

CommandRegistrar<MultiCommand> MultiCommand::registrar("MULTI");

std::vector<std::unique_ptr<RespType>>
MultiCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                          Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto &client_connection = static_cast<ClientConnection &>(connection);
  client_connection.beginTransaction();
  result.push_back(std::make_unique<RespString>("OK"));
  return result;
}
