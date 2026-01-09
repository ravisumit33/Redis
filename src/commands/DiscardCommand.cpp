#include "commands/DiscardCommand.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"

std::vector<RespValue>
DiscardCommand::executeOnImpl(const std::vector<RespValue> & /*args*/,
                              ClientConnection &connection) {
  std::vector<RespValue> result;
  if (!connection.isInTransaction()) {
    result.emplace_back(RespError("DISCARD without MULTI"));
    return result;
  }
  connection.discardTransaction();
  result.emplace_back(RespString("OK"));
  return result;
}
