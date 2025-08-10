#include "commands/WaitCommand.hpp"
#include "Connection.hpp"
#include "ReplicationState.hpp"
#include "RespType.hpp"

CommandRegistrar<WaitCommand> WaitCommand::registrar("WAIT");

std::vector<std::unique_ptr<RespType>>
WaitCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto arg1 = static_cast<RespBulkString &>(*args.at(0)).getValue();
  unsigned expected_slave_count = std::stoul(arg1);
  auto arg2 = static_cast<RespBulkString &>(*args.at(1)).getValue();
  unsigned timeout_ms = std::stoul(arg2);
  auto &master_state = ReplicationManager::getInstance().master();
  master_state.sendGetAckToSlaves();
  unsigned slave_count =
      master_state.waitForSlaves(expected_slave_count, timeout_ms);
  result.push_back(std::make_unique<RespInt>(slave_count));
  return result;
}
