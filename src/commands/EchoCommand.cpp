#include "commands/EchoCommand.hpp"
#include "Connection.hpp"
#include "RespType.hpp"

CommandRegistrar<EchoCommand> EchoCommand::registrar("ECHO");

std::vector<std::unique_ptr<RespType>>
EchoCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  result.push_back(args.at(0)->clone());
  return result;
}
