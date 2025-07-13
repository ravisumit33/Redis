#pragma once

#include "Registrar.hpp"
#include "Registry.hpp"
#include "RespType.hpp"
#include <memory>
#include <vector>

class Command {
public:
  virtual ~Command() = default;
  virtual std::unique_ptr<RespType>
  execute(std::vector<std::unique_ptr<RespType>> args) = 0;
};

using CommandRegistry = Registry<std::string, Command>;

template <typename T>
using CommandRegistrar = Registrar<std::string, Command, T>;

class EchoCommand : public Command {
public:
  virtual std::unique_ptr<RespType>
  execute(std::vector<std::unique_ptr<RespType>> args) override;

private:
  static CommandRegistrar<EchoCommand> registrar;
};

class PingCommand : public Command {
public:
  virtual std::unique_ptr<RespType>
  execute(std::vector<std::unique_ptr<RespType>> args) override;

private:
  static CommandRegistrar<PingCommand> registrar;
};

class SetCommand : public Command {
public:
  virtual std::unique_ptr<RespType>
  execute(std::vector<std::unique_ptr<RespType>> args) override;

private:
  static CommandRegistrar<SetCommand> registrar;
};

class GetCommand : public Command {
public:
  virtual std::unique_ptr<RespType>
  execute(std::vector<std::unique_ptr<RespType>> args) override;

private:
  static CommandRegistrar<GetCommand> registrar;
};
