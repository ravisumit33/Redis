#include "Command.hpp"
#include "AppConfig.hpp"
#include "RedisStore.hpp"
#include "ReplicationState.hpp"
#include "RespType.hpp"
#include "utils.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
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
EchoCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         const AppConfig &config, unsigned socket_fd) {
  std::vector<std::unique_ptr<RespType>> result;
  result.push_back(args.at(0)->clone());
  return result;
}

CommandRegistrar<PingCommand> PingCommand::registrar("PING");

std::vector<std::unique_ptr<RespType>>
PingCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         const AppConfig &config, unsigned socket_fd) {
  std::vector<std::unique_ptr<RespType>> result;
  result.push_back(std::make_unique<RespString>("PONG"));
  return result;
}

CommandRegistrar<SetCommand> SetCommand::registrar("SET");

std::vector<std::unique_ptr<RespType>>
SetCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                        const AppConfig &config, unsigned socket_fd) {
  std::vector<std::unique_ptr<RespType>> result;

  size_t nargs = args.size();
  auto key = static_cast<RespBulkString &>(*args.at(0)).getValue();
  auto value = static_cast<RespBulkString &>(*args.at(1)).getValue();
  std::optional<std::chrono::milliseconds> expiry = std::nullopt;
  if (nargs == 4) {
    auto expiryArgName = static_cast<RespBulkString &>(*args.at(2)).getValue();
    if (expiryArgName != "px") {
      result.push_back(std::make_unique<RespError>("Unsupported command arg"));
      return result;
    }
    try {
      auto expiry_time_str =
          static_cast<RespBulkString &>(*args.at(3)).getValue();
      auto expiryDelta = std::stoul(expiry_time_str);
      expiry = std::chrono::milliseconds(expiryDelta);
    } catch (const std::exception &ex) {
      auto expiry_time_str =
          static_cast<RespBulkString &>(*args.at(3)).getValue();
      std::cerr << "SET command: Invalid expiry time '" << expiry_time_str
                << "': " << ex.what() << std::endl;
      result.push_back(std::make_unique<RespError>("Unsupported command arg"));
      return result;
    }
  }

  RedisStore::instance().setString(key, value, expiry);

  result.push_back(std::make_unique<RespString>("OK"));
  return result;
}

CommandRegistrar<GetCommand> GetCommand::registrar("GET");

std::vector<std::unique_ptr<RespType>>
GetCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                        const AppConfig &config, unsigned socket_fd) {
  std::vector<std::unique_ptr<RespType>> result;

  auto key = static_cast<RespBulkString &>(*args.at(0)).getValue();

  auto stored_value = RedisStore::instance().get(key);

  if (!stored_value) {
    result.push_back(std::make_unique<RespBulkString>(""));
    return result;
  }

  auto return_value = std::move(stored_value.value());
  result.push_back(std::make_unique<RespBulkString>(return_value->serialize()));
  return result;
}

CommandRegistrar<InfoCommand> InfoCommand::registrar("INFO");

std::vector<std::unique_ptr<RespType>>
InfoCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         const AppConfig &config, unsigned socket_fd) {
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
ReplConfCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                             const AppConfig &config, unsigned socket_fd) {
  std::vector<std::unique_ptr<RespType>> result;
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

CommandRegistrar<PsyncCommand> PsyncCommand::registrar("PSYNC");

std::vector<std::unique_ptr<RespType>>
PsyncCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                          const AppConfig &config, unsigned socket_fd) {
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

CommandRegistrar<WaitCommand> WaitCommand::registrar("WAIT");

std::vector<std::unique_ptr<RespType>>
WaitCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         const AppConfig &config, unsigned socket_fd) {
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

CommandRegistrar<TypeCommand> TypeCommand::registrar("TYPE");

std::vector<std::unique_ptr<RespType>>
TypeCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         const AppConfig &config, unsigned socket_fd) {
  std::vector<std::unique_ptr<RespType>> result;
  auto arg1 = static_cast<RespBulkString &>(*args.at(0)).getValue();
  auto store_val_ptr = RedisStore::instance().get(arg1);
  if (!store_val_ptr) {
    result.push_back(std::make_unique<RespString>("none"));
    return result;
  }
  auto val_type = store_val_ptr.value()->getTypeStr();
  result.push_back(std::make_unique<RespString>(val_type));
  return result;
}

CommandRegistrar<XaddCommand> XaddCommand::registrar("XADD");

std::vector<std::unique_ptr<RespType>>
XaddCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         const AppConfig &config, unsigned socket_fd) {

  std::vector<std::unique_ptr<RespType>> result;
  auto store_key = static_cast<RespBulkString &>(*args.at(0)).getValue();
  auto stream_entry_id = static_cast<RespBulkString &>(*args.at(1)).getValue();

  StreamValue::StreamEntry stream_entry;
  std::size_t nargs = args.size();
  for (int i = 2; i < nargs; i += 2) {
    auto entry_key = static_cast<RespBulkString &>(*args.at(i)).getValue();
    auto entry_val = static_cast<RespBulkString &>(*args.at(i + 1)).getValue();
    stream_entry[entry_key] = entry_val;
  }

  try {
    auto saved_entry_id = RedisStore::instance().addStreamEntry(
        store_key, stream_entry_id, std::move(stream_entry));
    result.push_back(std::make_unique<RespBulkString>(
        std::to_string(saved_entry_id.at(0)) + "-" +
        std::to_string(saved_entry_id.at(1))));
    return result;
  } catch (const std::exception &ex) {
    result.push_back(std::make_unique<RespError>(ex.what()));
    return result;
  }
}

CommandRegistrar<XrangeCommand> XrangeCommand::registrar("XRANGE");

std::vector<std::unique_ptr<RespType>>
XrangeCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                           const AppConfig &config, unsigned socket_fd) {

  std::vector<std::unique_ptr<RespType>> result;
  auto store_key = static_cast<RespBulkString &>(*args.at(0)).getValue();

  auto store_val_ptr = RedisStore::instance().get(store_key);
  if (!store_val_ptr ||
      (*store_val_ptr)->getType() != RedisStoreValue::STREAM) {
    result.push_back(std::make_unique<RespArray>());
    return result;
  }

  auto stream_entry_id_start =
      static_cast<RespBulkString &>(*args.at(1)).getValue();
  auto stream_entry_id_end =
      static_cast<RespBulkString &>(*args.at(2)).getValue();

  auto stream_val = static_cast<StreamValue &>(**store_val_ptr);
  StreamValue::StreamIterator it1, it2;
  if (stream_entry_id_start == "-") {
    it1 = stream_val.begin();
  } else {
    auto entry_id_start = parseStreamEntryId(stream_entry_id_start);
    it1 = stream_val.lowerBound(entry_id_start);
  }
  if (stream_entry_id_end == "+") {
    it2 = stream_val.end();
  } else {
    auto entry_id_end = parseStreamEntryId(stream_entry_id_end);
    it2 = stream_val.upperBound(entry_id_end);
  }

  result.push_back(stream_val.serializeRangeIntoResp(it1, it2));
  return result;
}

CommandRegistrar<XreadCommand> XreadCommand::registrar("XREAD");

std::vector<std::unique_ptr<RespType>>
XreadCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                          const AppConfig &config, unsigned socket_fd) {

  std::vector<std::unique_ptr<RespType>> result;
  auto read_type = static_cast<RespBulkString &>(*args.at(0)).getValue();

  if (read_type != "streams") {
    result.push_back(
        std::make_unique<RespError>("Unsupported read_type arg: " + read_type));
    return result;
  }
  auto resp_array = std::make_unique<RespArray>();

  std::size_t pairs_count = (args.size() - 1) / 2;
  for (std::size_t i = 0; i < pairs_count; ++i) {
    auto store_key = static_cast<RespBulkString &>(*args.at(1 + i)).getValue();

    auto store_val_ptr = RedisStore::instance().get(store_key);
    if (!store_val_ptr ||
        (*store_val_ptr)->getType() != RedisStoreValue::STREAM) {
      result.push_back(std::make_unique<RespArray>());
      return result;
    }

    auto stream_val = static_cast<StreamValue &>(**store_val_ptr);
    auto stream_entry_id_start =
        static_cast<RespBulkString &>(*args.at(1 + i + pairs_count)).getValue();
    auto entry_id_start = parseStreamEntryId(stream_entry_id_start);
    StreamValue::StreamIterator it = stream_val.upperBound(entry_id_start);
    auto key_array = std::make_unique<RespArray>();
    key_array->add(std::make_unique<RespBulkString>(store_key))
        ->add(stream_val.serializeRangeIntoResp(it));
    resp_array->add(std::move(key_array));
  }

  result.push_back(std::move(resp_array));
  return result;
}
