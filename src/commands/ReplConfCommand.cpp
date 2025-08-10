#include "commands/ReplConfCommand.hpp"
#include "AppConfig.hpp"
#include "Connection.hpp"
#include "ReplicationState.hpp"
#include "RespType.hpp"
#include <iostream>

CommandRegistrar<ReplConfCommand> ReplConfCommand::registrar("REPLCONF");

std::vector<std::unique_ptr<RespType>>
ReplConfCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                             Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  const AppConfig &config = connection.getConfig();
  unsigned socket_fd = connection.getSocketFd();

  if (config.isMaster()) {
    auto arg1 = static_cast<RespBulkString &>(*args.at(0)).getValue();
    auto arg2 = static_cast<RespBulkString &>(*args.at(1)).getValue();
    if (arg1 == "ACK") {
      uint64_t slave_offset;
      try {
        slave_offset = std::stoull(arg2);
      } catch (const std::exception &ex) {
        std::cerr << "Error getting offset in REPLCONF ACK: " << ex.what()
                  << std::endl;
        result.push_back(
            std::make_unique<RespError>("Unsupported command arg"));
        return result;
      }
      auto &master_state = ReplicationManager::getInstance().master();
      master_state.updateSlave(socket_fd, slave_offset);
    } else {
      result.push_back(std::make_unique<RespString>("OK"));
    }
  } else {
    auto arg1 = static_cast<RespBulkString &>(*args.at(0)).getValue();
    auto arg2 = static_cast<RespBulkString &>(*args.at(1)).getValue();
    if (arg1 != "GETACK" || arg2 != "*") {
      result.push_back(std::make_unique<RespError>("Unsupported command arg"));
      return result;
    }
    auto resp_array = std::make_unique<RespArray>();
    auto &slave_state = ReplicationManager::getInstance().slave();
    std::size_t bytes_processed = slave_state.getBytesProcessed();
    resp_array->add(std::make_unique<RespBulkString>("REPLCONF"))
        ->add(std::make_unique<RespBulkString>("ACK"))
        ->add(
            std::make_unique<RespBulkString>(std::to_string(bytes_processed)));
    result.push_back(std::move(resp_array));
  }
  return result;
}
