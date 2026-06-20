#include "commands/MultiCommand.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include <vector>

std::vector<RespValue>
MultiCommand::execute(const std::vector<RespValue> & /*args*/,
                      ClientConnection &connection) {
  std::vector<RespValue> result;
  connection.beginTransaction();
  result.emplace_back(RespString("OK"));
  return result;
}
