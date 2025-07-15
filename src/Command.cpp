#include "Command.hpp"
#include "AppConfig.hpp"
#include "RedisStore.hpp"
#include "RespType.hpp"
#include "utils.hpp"
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

CommandRegistrar<EchoCommand> EchoCommand::registrar("ECHO");

std::vector<std::unique_ptr<RespType>>
EchoCommand::execute(std::vector<std::unique_ptr<RespType>> args,
                     const AppConfig &config) {
  std::vector<std::unique_ptr<RespType>> result;
  if (args.size() != 1) {
    result.push_back(std::make_unique<RespError>("Unexpected number of args"));
    return result;
  }

  if (args[0]->getType() != RespType::BULK_STRING) {
    result.push_back(std::make_unique<RespError>("Unexpected argument type"));
    return result;
  }

  result.push_back(std::move(args[0]));
  return result;
}

CommandRegistrar<PingCommand> PingCommand::registrar("PING");

std::vector<std::unique_ptr<RespType>>
PingCommand::execute(std::vector<std::unique_ptr<RespType>> args,
                     const AppConfig &config) {
  std::vector<std::unique_ptr<RespType>> result;
  if (args.size() != 0) {
    result.push_back(std::make_unique<RespError>("Unexpected number of args"));
    return result;
  }

  result.push_back(std::make_unique<RespBulkString>("PONG"));
  return result;
}

CommandRegistrar<SetCommand> SetCommand::registrar("SET");

std::vector<std::unique_ptr<RespType>>
SetCommand::execute(std::vector<std::unique_ptr<RespType>> args,
                    const AppConfig &config) {
  std::vector<std::unique_ptr<RespType>> result;
  if (args.size() != 4 && args.size() != 2) {
    result.push_back(std::make_unique<RespError>("Unexpected number of args"));
    return result;
  }

  size_t nargs = args.size();

  for (int i = 0; i < nargs; ++i) {
    if (args[i]->getType() != RespType::BULK_STRING) {
      result.push_back(
          std::make_unique<RespError>("Unexpected arguments type"));
      return result;
    }
  }

  auto key = static_cast<RespBulkString &>(*args.at(0)).getValue();
  auto value = static_cast<RespBulkString &>(*args.at(1)).getValue();

  std::optional<std::chrono::milliseconds> expiry = std::nullopt;
  if (nargs == 4) {
    auto expiryArgName = static_cast<RespBulkString &>(*args.at(2)).getValue();
    if (expiryArgName != "px") {
      result.push_back(std::make_unique<RespError>("Unexpected syntax"));
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
GetCommand::execute(std::vector<std::unique_ptr<RespType>> args,
                    const AppConfig &config) {
  std::vector<std::unique_ptr<RespType>> result;
  if (args.size() != 1) {
    result.push_back(std::make_unique<RespError>("Unexpected number of args"));
    return result;
  }

  if (args[0]->getType() != RespType::BULK_STRING) {
    result.push_back(std::make_unique<RespError>("Unexpected argument type"));
    return result;
  }

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
InfoCommand::execute(std::vector<std::unique_ptr<RespType>> args,
                     const AppConfig &config) {
  std::vector<std::unique_ptr<RespType>> result;
  if (args.size() != 1) {
    result.push_back(std::make_unique<RespError>("Unexpected number of args"));
    return result;
  }

  if (args.at(0)->getType() != RespType::BULK_STRING) {
    result.push_back(std::make_unique<RespError>("Unexpected argument type"));
    return result;
  }

  auto arg = static_cast<RespBulkString &>(*args.at(0)).getValue();
  if (arg != "replication") {
    result.push_back(std::make_unique<RespError>("Unsupported command arg"));
    return result;
  }

  if (config.getSlaveMetadata()) {
    result.push_back(std::make_unique<RespBulkString>("role:slave"));
    return result;
  }
  std::string replicationInfo = "role:master";
  auto [master_replid, master_repl_offset] = *config.getMasterMetadata();
  replicationInfo += std::string("\n") + "master_replid:" + master_replid;
  replicationInfo += std::string("\n") +
                     "master_repl_offset:" + std::to_string(master_repl_offset);
  result.push_back(std::make_unique<RespBulkString>(replicationInfo));
  return result;
}

CommandRegistrar<ReplConfCommand> ReplConfCommand::registrar("REPLCONF");

std::vector<std::unique_ptr<RespType>>
ReplConfCommand::execute(std::vector<std::unique_ptr<RespType>> args,
                         const AppConfig &config) {
  std::vector<std::unique_ptr<RespType>> result;
  result.push_back(std::make_unique<RespString>("OK"));
  return result;
}

CommandRegistrar<PsyncCommand> PsyncCommand::registrar("PSYNC");

std::vector<std::unique_ptr<RespType>>
PsyncCommand::execute(std::vector<std::unique_ptr<RespType>> args,
                      const AppConfig &config) {
  std::vector<std::unique_ptr<RespType>> result;
  auto [master_replid, master_repl_offset] = *config.getMasterMetadata();
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
