#include "commands/ReplConfCommand.hpp"
#include "AppConfig.hpp"
#include "AppContext.hpp"
#include "ReplicationState.hpp"
#include "RespType.hpp"
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"
#include <iostream>

std::vector<RespValue>
ReplConfCommand::doExecute(const std::vector<RespValue> &args,
                           AppContext &context, unsigned socket_fd) {
  std::vector<RespValue> result;
  const AppConfig &config = context.getConfig();

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
      auto &master_state = context.getReplicationManager().master();
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
    auto &slave_state = context.getReplicationManager().slave();
    std::size_t bytes_processed = slave_state.getBytesProcessed();
    resp_array.add(RespBulkString("REPLCONF"))
        .add(RespBulkString("ACK"))
        .add(RespBulkString(std::to_string(bytes_processed)));
    result.emplace_back(std::move(resp_array));
  }
  return result;
}

std::vector<RespValue>
ReplConfCommand::executeOnImpl(const std::vector<RespValue> &args,
                               ClientConnection &connection) {
  return doExecute(args, connection.getContext(), connection.getSocketFd());
}

std::vector<RespValue>
ReplConfCommand::executeOnImpl(const std::vector<RespValue> &args,
                               ServerConnection &connection) {
  return doExecute(args, connection.getContext(), connection.getSocketFd());
}
