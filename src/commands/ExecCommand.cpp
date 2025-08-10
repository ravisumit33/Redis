#include "commands/ExecCommand.hpp"
#include "Connection.hpp"
#include "RespType.hpp"

CommandRegistrar<ExecCommand> ExecCommand::registrar("EXEC");

std::vector<std::unique_ptr<RespType>>
ExecCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto &client_connection = static_cast<ClientConnection &>(connection);
  if (!client_connection.isInTransaction()) {
    result.push_back(std::make_unique<RespError>("EXEC without MULTI"));
    return result;
  }
  result.push_back(client_connection.executeTransaction());
  return result;
}
