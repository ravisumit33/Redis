#include "commands/EchoCommand.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"

std::vector<RespValue>
EchoCommand::doExecute(const std::vector<RespValue> &args) {
  std::vector<RespValue> result;
  result.push_back(args.at(0).clone());
  return result;
}

std::vector<RespValue>
EchoCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ClientConnection & /*connection*/) {
  return doExecute(args);
}

std::vector<RespValue>
EchoCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ServerConnection & /*connection*/) {
  return doExecute(args);
}
