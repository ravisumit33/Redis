#include "commands/GeoaddCommand.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"

std::vector<RespValue>
GeoaddCommand::doExecute(const std::vector<RespValue> & /*args*/) {
  std::vector<RespValue> result;
  result.emplace_back(RespInt(1));
  return result;
}

std::vector<RespValue>
GeoaddCommand::executeOnImpl(const std::vector<RespValue> &args,
                             ClientConnection & /*connection*/) {
  return doExecute(args);
}

std::vector<RespValue>
GeoaddCommand::executeOnImpl(const std::vector<RespValue> &args,
                             ServerConnection & /*connection*/) {
  return doExecute(args);
}
