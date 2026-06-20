#include "commands/EchoCommand.hpp"
#include "RespType.hpp"
#include <vector>

std::vector<RespValue>
EchoCommand::doExecute(const std::vector<RespValue> &args) {
  std::vector<RespValue> result;
  result.push_back(args.at(0).clone());
  return result;
}
