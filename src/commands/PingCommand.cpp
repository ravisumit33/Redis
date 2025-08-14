#include "commands/PingCommand.hpp"
#include "Connection.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include <memory>

CommandRegistrar<PingCommand> PingCommand::registrar("PING");

std::vector<std::unique_ptr<RespType>>
PingCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  if (connection.getType() == Connection::Type::CLIENT) {
    auto &client_connection = static_cast<ClientConnection &>(connection);
    if (client_connection.isInSubscribedMode()) {
      auto resp_array = std::make_unique<RespArray>();
      resp_array->add(std::make_unique<RespBulkString>("pong"))
          ->add(std::make_unique<RespBulkString>(""));
      result.push_back(std::move(resp_array));
      return result;
    }
  }
  result.push_back(std::make_unique<RespString>("PONG"));
  return result;
}
