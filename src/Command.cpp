#include "Command.hpp"
#include "RedisStore.hpp"
#include "RespType.hpp"
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

CommandRegistrar<EchoCommand> EchoCommand::registrar("ECHO");

std::unique_ptr<RespType>
EchoCommand::execute(std::vector<std::unique_ptr<RespType>> args,
                     const AppConfig &config) {
  if (args.size() != 1) {
    return std::make_unique<RespError>("Unexpected number of args");
  }

  if (args[0]->getType() != RespType::BULK_STRING) {
    return std::make_unique<RespError>("Unexpected argument type");
  }

  return std::move(args[0]);
}

CommandRegistrar<PingCommand> PingCommand::registrar("PING");

std::unique_ptr<RespType>
PingCommand::execute(std::vector<std::unique_ptr<RespType>> args,
                     const AppConfig &config) {
  if (args.size() != 0) {
    return std::make_unique<RespError>("Unexpected number of args");
  }

  return std::make_unique<RespBulkString>("PONG");
}

CommandRegistrar<SetCommand> SetCommand::registrar("SET");

std::unique_ptr<RespType>
SetCommand::execute(std::vector<std::unique_ptr<RespType>> args,
                    const AppConfig &config) {
  if (args.size() != 4 && args.size() != 2) {
    return std::make_unique<RespError>("Unexpected number of args");
  }

  size_t nargs = args.size();

  for (int i = 0; i < nargs; ++i) {
    if (args[i]->getType() != RespType::BULK_STRING) {
      return std::make_unique<RespError>("Unexpected arguments type");
    }
  }

  auto key = static_cast<RespBulkString &>(*args.at(0)).getValue();
  auto value = static_cast<RespBulkString &>(*args.at(1)).getValue();

  std::optional<std::chrono::milliseconds> expiry = std::nullopt;
  if (nargs == 4) {
    auto expiryArgName = static_cast<RespBulkString &>(*args.at(2)).getValue();
    if (expiryArgName != "px") {
      return std::make_unique<RespError>("Unexpected syntax");
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

  return std::make_unique<RespString>("OK");
}

CommandRegistrar<GetCommand> GetCommand::registrar("GET");

std::unique_ptr<RespType>
GetCommand::execute(std::vector<std::unique_ptr<RespType>> args,
                    const AppConfig &config) {
  if (args.size() != 1) {
    return std::make_unique<RespError>("Unexpected number of args");
  }

  if (args[0]->getType() != RespType::BULK_STRING) {
    return std::make_unique<RespError>("Unexpected argument type");
  }

  auto key = static_cast<RespBulkString &>(*args.at(0)).getValue();

  std::string return_value = "";
  auto stored_value = RedisStore::instance().get(key);

  if (!stored_value) {
    return std::make_unique<RespBulkString>(return_value);
  }

  return_value = stored_value.value();
  return std::make_unique<RespBulkString>(return_value);
}

CommandRegistrar<InfoCommand> InfoCommand::registrar("INFO");

std::unique_ptr<RespType>
InfoCommand::execute(std::vector<std::unique_ptr<RespType>> args,
                     const AppConfig &config) {
  if (args.size() != 1) {
    return std::make_unique<RespError>("Unexpected number of args");
  }

  if (args.at(0)->getType() != RespType::BULK_STRING) {
    return std::make_unique<RespError>("Unexpected argument type");
  }

  auto arg = static_cast<RespBulkString &>(*args.at(0)).getValue();
  if (arg != "replication") {
    return std::make_unique<RespError>("Unsupported command arg");
  }

  if (!config.replicaOf.empty()) {
    return std::make_unique<RespBulkString>("role:slave");
  }
  return std::make_unique<RespBulkString>("role:master");
}
