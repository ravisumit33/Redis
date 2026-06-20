#include "commands/WaitCommand.hpp"
#include "ReplicationState.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"

std::vector<RespValue>
WaitCommand::doExecute(const std::vector<RespValue> &args,
                       ReplicationManager &repl) {
  std::vector<RespValue> result;
  auto arg1 = getStringValue(args.at(0));
  unsigned expected_slave_count = std::stoul(arg1);
  auto arg2 = getStringValue(args.at(1));
  unsigned timeout_ms = std::stoul(arg2);
  auto &master_state = repl.master();
  master_state.sendGetAckToSlaves();
  unsigned slave_count =
      master_state.waitForSlaves(expected_slave_count, timeout_ms);
  result.emplace_back(RespInt(slave_count));
  return result;
}

std::vector<RespValue>
WaitCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ClientConnection &connection) {
  return doExecute(args, connection.getContext().getReplicationManager());
}

std::vector<RespValue>
WaitCommand::executeOnImpl(const std::vector<RespValue> &args,
                           ServerConnection &connection) {
  return doExecute(args, connection.getContext().getReplicationManager());
}
