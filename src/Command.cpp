#include "Command.hpp"
#include "Registrar.hpp"
#include "RespType.hpp"
#include <memory>
#include <string>
#include <vector>

Registrar<std::string, Command, EchoCommand> EchoCommand::registrar("ECHO");

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

Registrar<std::string, Command, PingCommand> PingCommand::registrar("PING");

std::unique_ptr<RespType>
PingCommand::execute(std::vector<std::unique_ptr<RespType>> args) {
  if (args.size() != 0) {
    return std::make_unique<RespError>("Unexpected number of args");
  }

  return std::make_unique<RespBulkString>("PONG");
}
