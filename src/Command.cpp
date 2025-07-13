#include "Command.hpp"
#include "RedisStore.hpp"
#include "RespType.hpp"
#include <memory>
#include <string>
#include <vector>

CommandRegistrar<EchoCommand> EchoCommand::registrar("ECHO");

std::unique_ptr<RespType>
EchoCommand::execute(std::vector<std::unique_ptr<RespType>> args) {
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
PingCommand::execute(std::vector<std::unique_ptr<RespType>> args) {
  if (args.size() != 0) {
    return std::make_unique<RespError>("Unexpected number of args");
  }

  return std::make_unique<RespBulkString>("PONG");
}

CommandRegistrar<SetCommand> SetCommand::registrar("SET");

std::unique_ptr<RespType>
SetCommand::execute(std::vector<std::unique_ptr<RespType>> args) {
  if (args.size() != 2) {
    return std::make_unique<RespError>("Unexpected number of args");
  }

  if (args[0]->getType() != RespType::BULK_STRING ||
      args[1]->getType() != RespType::BULK_STRING) {
    return std::make_unique<RespError>("Unexpected arguments type");
  }

  auto key = static_cast<RespBulkString &>(*args[0]).getValue();
  auto value = static_cast<RespBulkString &>(*args[1]).getValue();

  RedisStore::instance().set(key, value);

  return std::make_unique<RespString>("OK");
}

CommandRegistrar<GetCommand> GetCommand::registrar("GET");

std::unique_ptr<RespType>
GetCommand::execute(std::vector<std::unique_ptr<RespType>> args) {
  if (args.size() != 1) {
    return std::make_unique<RespError>("Unexpected number of args");
  }

  if (args[0]->getType() != RespType::BULK_STRING) {
    return std::make_unique<RespError>("Unexpected argument type");
  }

  auto key = static_cast<RespBulkString &>(*args[0]).getValue();

  std::string return_value = "";
  auto stored_value = RedisStore::instance().get(key);

  if (!stored_value) {
    return std::make_unique<RespBulkString>(return_value);
  }

  return_value = stored_value.value();
  return std::make_unique<RespBulkString>(return_value);
}
