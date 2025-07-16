#include "Command.hpp"
#include "AppConfig.hpp"
#include "RedisStore.hpp"
#include "ReplicationState.hpp"
#include "RespType.hpp"
#include "utils.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

bool Command::isWriteCommand() const {
  static std::array<Type, 1> write_commands = {SET};
  return std::any_of(write_commands.begin(), write_commands.end(),
                     [this](Type &t) { return t == getType(); });
}

CommandRegistrar<EchoCommand> EchoCommand::registrar("ECHO");

std::vector<std::unique_ptr<RespType>>
EchoCommand::executeImpl(std::vector<std::unique_ptr<RespType>> args,
                         const AppConfig &config) {
  std::vector<std::unique_ptr<RespType>> result;
  result.push_back(std::move(args.at(0)));
  return result;
}

CommandRegistrar<PingCommand> PingCommand::registrar("PING");

std::vector<std::unique_ptr<RespType>>
PingCommand::executeImpl(std::vector<std::unique_ptr<RespType>> args,
                         const AppConfig &config) {
  std::vector<std::unique_ptr<RespType>> result;
  result.push_back(std::make_unique<RespBulkString>("PONG"));
  return result;
}

CommandRegistrar<SetCommand> SetCommand::registrar("SET");

std::vector<std::unique_ptr<RespType>>
SetCommand::executeImpl(std::vector<std::unique_ptr<RespType>> args,
                        const AppConfig &config) {
  std::vector<std::unique_ptr<RespType>> result;

  size_t nargs = args.size();
  auto key = static_cast<RespBulkString &>(*args.at(0)).getValue();
  auto value = static_cast<RespBulkString &>(*args.at(1)).getValue();
  std::optional<std::chrono::milliseconds> expiry = std::nullopt;
  if (nargs == 4) {
    auto expiryArgName = static_cast<RespBulkString &>(*args.at(2)).getValue();
    if (expiryArgName != "px") {
      result.push_back(std::make_unique<RespError>("Unsupported command arg"));
    }
    try {
      auto expiryDelta =
          std::stoul(static_cast<RespBulkString &>(*args.at(3)).getValue());
      expiry = std::chrono::milliseconds(expiryDelta);
    } catch (const std::exception &ex) {
      std::cerr << "Unexpected expiry time: " << ex.what() << std::endl;
    }
  }

  RedisStore::instance().set(key, value, expiry);

  result.push_back(std::make_unique<RespString>("OK"));
  return result;
}

CommandRegistrar<GetCommand> GetCommand::registrar("GET");

std::vector<std::unique_ptr<RespType>>
GetCommand::executeImpl(std::vector<std::unique_ptr<RespType>> args,
                        const AppConfig &config) {
  std::vector<std::unique_ptr<RespType>> result;

  auto key = static_cast<RespBulkString &>(*args.at(0)).getValue();

  std::string return_value = "";
  auto stored_value = RedisStore::instance().get(key);

  if (!stored_value) {
    result.push_back(std::make_unique<RespBulkString>(return_value));
    return result;
  }

  return_value = stored_value.value();
  result.push_back(std::make_unique<RespBulkString>(return_value));
  return result;
}

CommandRegistrar<InfoCommand> InfoCommand::registrar("INFO");

std::vector<std::unique_ptr<RespType>>
InfoCommand::executeImpl(std::vector<std::unique_ptr<RespType>> args,
                         const AppConfig &config) {
  std::vector<std::unique_ptr<RespType>> result;

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

CommandRegistrar<ReplConfCommand> ReplConfCommand::registrar("REPLCONF");

std::vector<std::unique_ptr<RespType>>
ReplConfCommand::executeImpl(std::vector<std::unique_ptr<RespType>> args,
                             const AppConfig &config) {
  std::vector<std::unique_ptr<RespType>> result;
  result.push_back(std::make_unique<RespString>("OK"));
  return result;
}

CommandRegistrar<PsyncCommand> PsyncCommand::registrar("PSYNC");

std::vector<std::unique_ptr<RespType>>
PsyncCommand::executeImpl(std::vector<std::unique_ptr<RespType>> args,
                          const AppConfig &config) {
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
  result.push_back(std::make_unique<RespRDB>(empty_rdb_binary));
  return result;
}
