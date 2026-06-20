#include "commands/ReplConfCommand.hpp"
#include "AppConfig.hpp"
#include "ReplicationState.hpp"
#include "RespType.hpp"
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

std::vector<RespValue>
ReplConfCommand::doExecute(const std::vector<RespValue> &args,
                           const AppConfig &config, ReplicationManager &repl,
                           unsigned socket_fd) {
  std::vector<RespValue> result;

  if (config.isMaster()) {
    auto arg1 = getStringValue(args.at(0));
    auto arg2 = getStringValue(args.at(1));
    if (arg1 == "ACK") {
      uint64_t slave_offset{};
      try {
        slave_offset = std::stoull(arg2);
      } catch (const std::exception &ex) {
        std::cerr << "Error getting offset in REPLCONF ACK: " << ex.what()
                  << '\n';
        result.emplace_back(RespError("Unsupported command arg"));
        return result;
      }
      auto &master_state = repl.master();
      master_state.updateSlave(socket_fd, slave_offset);
    } else {
      result.emplace_back(RespString("OK"));
    }
  } else {
    auto arg1 = getStringValue(args.at(0));
    auto arg2 = getStringValue(args.at(1));
    if (arg1 != "GETACK" || arg2 != "*") {
      result.emplace_back(RespError("Unsupported command arg"));
      return result;
    }
    RespArray resp_array;
    auto &slave_state = repl.slave();
    const std::size_t bytes_processed = slave_state.getBytesProcessed();
    resp_array.add(RespBulkString("REPLCONF"))
        .add(RespBulkString("ACK"))
        .add(RespBulkString(std::to_string(bytes_processed)));
    result.emplace_back(std::move(resp_array));
  }
  return result;
}
