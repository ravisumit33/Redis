#include "commands/ConfigCommand.hpp"
#include "AppConfig.hpp"
#include "Connection.hpp"
#include "RespType.hpp"

CommandRegistrar<ConfigCommand> ConfigCommand::registrar("CONFIG");

std::vector<std::unique_ptr<RespType>>
ConfigCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                           Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto arg1 = static_cast<RespBulkString &>(*args.at(0)).getValue();
  if (arg1 != "GET") {
    result.push_back(std::make_unique<RespError>("Invalid argument: " + arg1));
    return result;
  }
  auto arg2 = static_cast<RespBulkString &>(*args.at(1)).getValue();
  auto &config = connection.getConfig();
  auto &master_config = config.getMasterConfig();
  if (!master_config) {
    result.push_back(std::make_unique<RespError>(
        "Command not supported in non-master mode"));
    return result;
  }
  if (arg2 != "dir") {
    result.push_back(std::make_unique<RespError>("Unknown config: " + arg2));
    return result;
  }
  auto resp_array = std::make_unique<RespArray>();
  resp_array->add(std::make_unique<RespBulkString>(arg2))
      ->add(std::make_unique<RespBulkString>(master_config->dir));
  result.push_back(std::move(resp_array));
  return result;
}
