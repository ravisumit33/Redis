#include "commands/ExecCommand.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include <vector>

std::vector<RespValue>
ExecCommand::execute(const std::vector<RespValue> & /*args*/,
                     ClientConnection &connection) {
  std::vector<RespValue> result;
  if (!connection.isInTransaction()) {
    result.emplace_back(RespError("EXEC without MULTI"));
    return result;
  }
  result.emplace_back(connection.executeTransaction());
  return result;
}
