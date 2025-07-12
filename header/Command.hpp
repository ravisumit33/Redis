#pragma once

#include "Registrar.hpp"
#include "RespType.hpp"
#include <memory>
#include <vector>

class Command {
public:
  virtual ~Command() = default;
  virtual std::unique_ptr<RespType>
  execute(std::vector<std::unique_ptr<RespType>> args) = 0;
};

class EchoCommand : public Command {
public:
  virtual std::unique_ptr<RespType>
  execute(std::vector<std::unique_ptr<RespType>> args) override;

private:
  static Registrar<std::string, Command, EchoCommand> registrar;
};

class PingCommand : public Command {
public:
  virtual std::unique_ptr<RespType>
  execute(std::vector<std::unique_ptr<RespType>> args) override;

private:
  static Registrar<std::string, Command, PingCommand> registrar;
};
