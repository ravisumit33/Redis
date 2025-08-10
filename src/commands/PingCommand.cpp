#include "commands/PingCommand.hpp"
#include "Connection.hpp"
#include "RespType.hpp"

CommandRegistrar<PingCommand> PingCommand::registrar("PING");

std::vector<std::unique_ptr<RespType>>
PingCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  result.push_back(std::make_unique<RespString>("PONG"));
  return result;
}
