#include "commands/InfoCommand.hpp"
#include "AppConfig.hpp"
#include "ReplicationState.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"
#include <string>

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
  std::string master_replid = master_state.getReplId();
  uint64_t master_repl_offset = master_state.getReplOffset();
  replicationInfo += std::string("\n") + "master_replid:" + master_replid;
  replicationInfo += std::string("\n") +
                     "master_repl_offset:" + std::to_string(master_repl_offset);
  result.emplace_back(RespBulkString(replicationInfo));
  return result;
}

std::vector<RespValue>
InfoCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ClientConnection &connection) {
  return doExecute(args, connection.getContext().getConfig(),
                   connection.getContext().getReplicationManager());
}

std::vector<RespValue>
InfoCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ServerConnection &connection) {
  return doExecute(args, connection.getContext().getConfig(),
                   connection.getContext().getReplicationManager());
}
