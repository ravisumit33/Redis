#include "commands/PsyncCommand.hpp"
#include "Connection.hpp"
#include "ReplicationState.hpp"
#include "RespType.hpp"
#include "utils/genericUtils.hpp"
#include <string>

CommandRegistrar<PsyncCommand> PsyncCommand::registrar("PSYNC");

std::vector<std::unique_ptr<RespType>>
PsyncCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                          Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;

  auto arg = static_cast<RespBulkString &>(*args.at(0)).getValue();
  if (arg != "?") {
    result.push_back(std::make_unique<RespError>("Unsupported command arg"));
    return result;
  }
  arg = static_cast<RespBulkString &>(*args.at(1)).getValue();
  if (arg != "-1") {
    result.push_back(std::make_unique<RespError>("Unsupported command arg"));
    return result;
  }

  auto &master_state = ReplicationManager::getInstance().master();
  std::string master_replid = master_state.getReplId();
  uint64_t master_repl_offset = master_state.getReplOffset();
  result.push_back(
      std::make_unique<RespString>("FULLRESYNC " + master_replid + " " +
                                   std::to_string(master_repl_offset)));

  const std::string empty_rdb_hex =
      "524544495330303131fa0972656469732d76657205372e322e30fa0a72656469732d6269"
      "7473c040fa056374696d65c26d08bc65fa08757365642d6d656dc2b0c41000fa08616f66"
      "2d62617365c000fff06e3bfec0ff5aa2";

  const std::string empty_rdb_binary = hexToBinary(empty_rdb_hex);
  result.push_back(std::make_unique<RespBulkString>(empty_rdb_binary, false));
  return result;
}
