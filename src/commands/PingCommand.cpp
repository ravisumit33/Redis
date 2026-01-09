#include "commands/PingCommand.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"

std::vector<RespValue>
PingCommand::executeOnImpl(const std::vector<RespValue> & /*args*/,
                           ClientConnection &connection) {
  std::vector<RespValue> result;
  if (connection.isInSubscribedMode()) {
    RespArray resp_array;
    resp_array.add(RespBulkString("pong")).add(RespBulkString(""));
    result.emplace_back(std::move(resp_array));
    return result;
  }
  result.emplace_back(RespString("PONG"));
  return result;
}

std::vector<RespValue>
PingCommand::executeOnImpl(const std::vector<RespValue> & /*args*/,
                           ServerConnection & /*connection*/) {
  std::vector<RespValue> result;
  result.emplace_back(RespString("PONG"));
  return result;
}
