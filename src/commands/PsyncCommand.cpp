#include "commands/PsyncCommand.hpp"
#include "ReplicationState.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"
#include "utils/genericUtils.hpp"
#include <string>

std::vector<RespValue>
PsyncCommand::doExecute(const std::vector<RespValue> &args,
                        ReplicationManager &repl) {
  std::vector<RespValue> result;

  auto arg = getStringValue(args.at(0));
  if (arg != "?") {
    result.emplace_back(RespError("Unsupported command arg"));
    return result;
  }
  arg = getStringValue(args.at(1));
  if (arg != "-1") {
    result.emplace_back(RespError("Unsupported command arg"));
    return result;
  }

  auto &master_state = repl.master();
  std::string master_replid = master_state.getReplId();
  uint64_t master_repl_offset = master_state.getReplOffset();
  result.emplace_back(RespString("FULLRESYNC " + master_replid + " " +
                                 std::to_string(master_repl_offset)));

  const std::string empty_rdb_hex =
      "524544495330303131fa0972656469732d76657205372e322e30fa0a72656469732d6269"
      "7473c040fa056374696d65c26d08bc65fa08757365642d6d656dc2b0c41000fa08616f66"
      "2d62617365c000fff06e3bfec0ff5aa2";

  const std::string empty_rdb_binary = hexToBinary(empty_rdb_hex);
  result.emplace_back(RespBulkString(empty_rdb_binary, false));
  return result;
}

std::vector<RespValue>
PsyncCommand::executeOnImpl(const std::vector<RespValue> &args,
                            ClientConnection &connection) {
  return doExecute(args, connection.getContext().getReplicationManager());
}

std::vector<RespValue>
PsyncCommand::executeOnImpl(const std::vector<RespValue> &args,
                            ServerConnection &connection) {
  return doExecute(args, connection.getContext().getReplicationManager());
}
