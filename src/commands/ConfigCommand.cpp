#include "commands/ConfigCommand.hpp"
#include "AppConfig.hpp"
#include "RespType.hpp"
#include <utility>
#include <vector>

std::vector<RespValue>
ConfigCommand::doExecute(const std::vector<RespValue> &args,
                         const AppConfig &config) {
  std::vector<RespValue> result;
  auto arg1 = getStringValue(args.at(0));
  if (arg1 != "GET") {
    result.emplace_back(RespError("Invalid argument: " + arg1));
    return result;
  }
  auto arg2 = getStringValue(args.at(1));
  const auto &master_config = config.getMasterConfig();
  if (!master_config) {
    result.emplace_back(RespError("Command not supported in non-master mode"));
    return result;
  }
  if (arg2 != "dir") {
    result.emplace_back(RespError("Unknown config: " + arg2));
    return result;
  }
  RespArray resp_array;
  resp_array.add(RespBulkString(arg2)).add(RespBulkString(master_config->dir));
  result.emplace_back(std::move(resp_array));
  return result;
}
