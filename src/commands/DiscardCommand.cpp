#include "commands/DiscardCommand.hpp"
#include "Connection.hpp"
#include "RespType.hpp"

CommandRegistrar<DiscardCommand> DiscardCommand::registrar("DISCARD");

std::vector<std::unique_ptr<RespType>>
DiscardCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                            Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto &client_connection = static_cast<ClientConnection &>(connection);
  if (!client_connection.isInTransaction()) {
    result.push_back(std::make_unique<RespError>("DISCARD without MULTI"));
    return result;
  }
  client_connection.discardTransaction();
  result.push_back(std::make_unique<RespString>("OK"));
  return result;
}
