#include "Command.hpp"
#include "AppConfig.hpp"
#include "Connection.hpp"
#include "RedisStore.hpp"
#include "ReplicationState.hpp"
#include "RespType.hpp"
#include "utils.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

bool Command::isWriteCommand() const {
  static const std::unordered_set<Type> write_commands = {SET};
  return write_commands.contains(getType());
}

bool Command::isControlCommand() const {
  static const std::unordered_set<Type> control_commands = {EXEC, DISCARD};
  return control_commands.contains(getType());
}

CommandRegistrar<EchoCommand> EchoCommand::registrar("ECHO");

std::vector<std::unique_ptr<RespType>>
EchoCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  result.push_back(args.at(0)->clone());
  return result;
}

CommandRegistrar<PingCommand> PingCommand::registrar("PING");

std::vector<std::unique_ptr<RespType>>
PingCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  result.push_back(std::make_unique<RespString>("PONG"));
  return result;
}

CommandRegistrar<SetCommand> SetCommand::registrar("SET");

std::vector<std::unique_ptr<RespType>>
SetCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                        Connection &connection) {
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
                        Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;

  auto key = static_cast<RespBulkString &>(*args.at(0)).getValue();

  auto stored_value = RedisStore::instance().get(key);

  if (!stored_value) {
    result.push_back(std::make_unique<RespBulkString>(""));
    return result;
  }

  auto return_value = std::move(stored_value.value());
  if (return_value->getType() != RedisStoreValue::STRING) {
    result.push_back(std::make_unique<RespError>("Invalid Key"));
    return result;
  }
  result.push_back(std::make_unique<RespBulkString>(
      static_cast<StringValue &>(*return_value).getValue()));
  return result;
}

CommandRegistrar<InfoCommand> InfoCommand::registrar("INFO");

std::vector<std::unique_ptr<RespType>>
InfoCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  const AppConfig &config = connection.getConfig();

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

CommandRegistrar<TypeCommand> TypeCommand::registrar("TYPE");

std::vector<std::unique_ptr<RespType>>
TypeCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {
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
                         Connection &connection) {

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
                           Connection &connection) {

  std::vector<std::unique_ptr<RespType>> result;
  auto store_key = static_cast<RespBulkString &>(*args.at(0)).getValue();

  auto store_val_ptr = RedisStore::instance().get(store_key);
  if (!store_val_ptr ||
      (*store_val_ptr)->getType() != RedisStoreValue::STREAM) {
    result.push_back(
        std::make_unique<RespError>("Key doesn't contain a stream"));
    return result;
  }

  auto stream_entry_id_start =
      static_cast<RespBulkString &>(*args.at(1)).getValue();
  auto stream_entry_id_end =
      static_cast<RespBulkString &>(*args.at(2)).getValue();

  auto stream_entries = RedisStore::instance().getStreamEntriesInRange(
      store_key, stream_entry_id_start, stream_entry_id_end);
  result.push_back(serializeStreamEntries(std::move(stream_entries)));
  return result;
}

CommandRegistrar<XreadCommand> XreadCommand::registrar("XREAD");

std::vector<std::unique_ptr<RespType>>
XreadCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                          Connection &connection) {

  std::vector<std::unique_ptr<RespType>> result;
  std::size_t arg_idx = 0;
  std::optional<uint64_t> timeout_ms;
  auto first_arg = static_cast<RespBulkString &>(*args.at(arg_idx)).getValue();
  ++arg_idx;
  if (first_arg == "block") {
    timeout_ms = std::stoull(
        static_cast<RespBulkString &>(*args.at(arg_idx)).getValue());
    ++arg_idx;
    auto second_arg =
        static_cast<RespBulkString &>(*args.at(arg_idx)).getValue();
    ++arg_idx;

    if (second_arg != "streams") {
      result.push_back(std::make_unique<RespError>(
          "Unsupported read_type arg: " + second_arg));
      return result;
    }
  } else if (first_arg != "streams") {
    result.push_back(
        std::make_unique<RespError>("Unsupported read_type arg: " + first_arg));
    return result;
  }

  std::size_t pairs_count = (args.size() - arg_idx) / 2;
  std::vector<std::string> store_keys, entry_ids_start;
  for (std::size_t i = 0; i < pairs_count; ++i) {
    auto store_key =
        static_cast<RespBulkString &>(*args.at(arg_idx + i)).getValue();
    auto stream_entry_id_start =
        static_cast<RespBulkString &>(*args.at(arg_idx + i + pairs_count))
            .getValue();
    store_keys.push_back(store_key);
    entry_ids_start.push_back(stream_entry_id_start);
  }
  auto [timed_out, stream_entries_map] =
      RedisStore::instance().getStreamEntriesAfterAny(
          store_keys, entry_ids_start, timeout_ms);

  if (timed_out) {
    result.push_back(std::make_unique<RespBulkString>());
    return result;
  }

  auto resp_array = std::make_unique<RespArray>();

  for (const auto &store_key : store_keys) {
    if (stream_entries_map.contains(store_key)) {
      auto stream_entries = stream_entries_map[store_key];
      auto key_array = std::make_unique<RespArray>();
      key_array->add(std::make_unique<RespBulkString>(store_key))
          ->add(serializeStreamEntries(stream_entries));
      resp_array->add(std::move(key_array));
    }
  }

  result.push_back(std::move(resp_array));
  return result;
}

CommandRegistrar<IncrCommand> IncrCommand::registrar("INCR");

std::vector<std::unique_ptr<RespType>>
IncrCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {

  std::vector<std::unique_ptr<RespType>> result;
  auto store_key = static_cast<RespBulkString &>(*args.at(0)).getValue();

  auto store_val_ptr = RedisStore::instance().get(store_key);
  if (!store_val_ptr) {
    RedisStore::instance().setString(store_key, "1");
    result.push_back(std::make_unique<RespInt>(1));
    return result;
  }

  auto store_val =
      static_cast<StringValue &>(*(store_val_ptr.value())).getValue();
  int64_t int_val;
  try {
    int_val = std::stoll(store_val);
  } catch (...) {
    result.push_back(
        std::make_unique<RespError>("value is not an integer or out of range"));
    return result;
  }

  ++int_val;
  RedisStore::instance().setString(store_key, std::to_string(int_val));
  result.push_back(std::make_unique<RespInt>(int_val));
  return result;
}

CommandRegistrar<MultiCommand> MultiCommand::registrar("MULTI");

std::vector<std::unique_ptr<RespType>>
MultiCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                          Connection &connection) {

  std::vector<std::unique_ptr<RespType>> result;
  auto &client_connection = static_cast<ClientConnection &>(connection);
  client_connection.beginTransaction();
  result.push_back(std::make_unique<RespString>("OK"));
  return result;
}

CommandRegistrar<ExecCommand> ExecCommand::registrar("EXEC");

std::vector<std::unique_ptr<RespType>>
ExecCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {

  std::vector<std::unique_ptr<RespType>> result;
  auto &client_connection = static_cast<ClientConnection &>(connection);
  if (!client_connection.isInTransaction()) {
    result.push_back(std::make_unique<RespError>("EXEC without MULTI"));
    return result;
  }
  result.push_back(client_connection.executeTransaction());
  return result;
}

CommandRegistrar<DiscardCommand> DiscardCommand::registrar("DISCARD");

std::vector<std::unique_ptr<RespType>>
DiscardCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                            Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto &client_connection = static_cast<ClientConnection &>(connection);
  if (!client_connection.isInTransaction()) {
    result.push_back(std::make_unique<RespError>("DISCARD without MULTI"));
    return result;
  }
  client_connection.discardTransaction();
  result.push_back(std::make_unique<RespString>("OK"));
  return result;
}

CommandRegistrar<ConfigCommand> ConfigCommand::registrar("CONFIG");

std::vector<std::unique_ptr<RespType>>
ConfigCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                           Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto arg1 = static_cast<RespBulkString &>(*args.at(0)).getValue();
  if (arg1 != "GET") {
    result.push_back(std::make_unique<RespError>("Invalid argument: " + arg1));
    return result;
  }
  auto arg2 = static_cast<RespBulkString &>(*args.at(1)).getValue();
  auto &config = connection.getConfig();
  auto &master_config = config.getMasterConfig();
  if (!master_config) {
    result.push_back(std::make_unique<RespError>(
        "Command not supported in non-master mode"));
    return result;
  }
  if (arg2 != "dir") {
    result.push_back(std::make_unique<RespError>("Unknown config: " + arg2));
    return result;
  }
  auto resp_array = std::make_unique<RespArray>();
  resp_array->add(std::make_unique<RespBulkString>(arg2))
      ->add(std::make_unique<RespBulkString>(master_config->dir));
  result.push_back(std::move(resp_array));
  return result;
}

CommandRegistrar<KeysCommand> KeysCommand::registrar("KEYS");

std::vector<std::unique_ptr<RespType>>
KeysCommand::executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
                         Connection &connection) {
  std::vector<std::unique_ptr<RespType>> result;
  auto arg1 = static_cast<RespBulkString &>(*args.at(0)).getValue();
  if (arg1 != "*") {
    result.push_back(std::make_unique<RespError>("Invalid argument: " + arg1));
    return result;
  }
  auto store_keys = RedisStore::instance().getKeys();
  auto resp_array = std::make_unique<RespArray>();
  for (const auto &store_key : store_keys) {
    resp_array->add(std::make_unique<RespBulkString>(store_key));
  }
  result.push_back(std::move(resp_array));
  return result;
}
