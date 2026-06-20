#include "commands/InfoCommand.hpp"
#include "AppConfig.hpp"
#include "ReplicationState.hpp"
#include "RespType.hpp"
#include <cstdint>
#include <string>
#include <vector>

std::vector<RespValue>
InfoCommand::doExecute(const std::vector<RespValue> &args,
                       const AppConfig &config, ReplicationManager &repl) {
  std::vector<RespValue> result;

  auto arg = getStringValue(args.at(0));
  if (arg != "replication") {
    result.emplace_back(RespError("Unsupported command arg"));
    return result;
  }

  if (config.getSlaveConfig()) {
    result.emplace_back(RespBulkString("role:slave"));
    return result;
  }
  std::string replicationInfo = "role:master";
  auto &master_state = repl.master();
  const std::string master_replid = master_state.getReplId();
  const uint64_t master_repl_offset = master_state.getReplOffset();
  replicationInfo += std::string("\n") + "master_replid:" + master_replid;
  replicationInfo += std::string("\n") +
                     "master_repl_offset:" + std::to_string(master_repl_offset);
  result.emplace_back(RespBulkString(replicationInfo));
  return result;
}
