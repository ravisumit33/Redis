#include "commands/InfoCommand.hpp"
#include "AppConfig.hpp"
#include "Connection.hpp"
#include "ReplicationState.hpp"
#include "RespType.hpp"
#include <string>

CommandRegistrar<InfoCommand> InfoCommand::registrar("INFO");

std::vector<std::unique_ptr<RespType>>
InfoCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  const AppConfig &config = connection.getConfig();

  auto arg = static_cast<RespBulkString &>(*args.at(0)).getValue();
  if (arg != "replication") {
    result.push_back(std::make_unique<RespError>("Unsupported command arg"));
    return result;
  }

  if (config.getSlaveConfig()) {
    result.push_back(std::make_unique<RespBulkString>("role:slave"));
    return result;
  }
  std::string replicationInfo = "role:master";
  auto &master_state = ReplicationManager::getInstance().master();
  std::string master_replid = master_state.getReplId();
  uint64_t master_repl_offset = master_state.getReplOffset();
  replicationInfo += std::string("\n") + "master_replid:" + master_replid;
  replicationInfo += std::string("\n") +
                     "master_repl_offset:" + std::to_string(master_repl_offset);
  result.push_back(std::make_unique<RespBulkString>(replicationInfo));
  return result;
}
