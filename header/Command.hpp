#pragma once

#include "AppConfig.hpp"
#include "Registrar.hpp"
#include "Registry.hpp"
#include "RespType.hpp"
#include <memory>
#include <vector>

class Command {
public:
  virtual ~Command() = default;

  enum Type { ECHO, PING, SET, GET, INFO, REPLCONF, PSYNC, WAIT, TYPE };

  std::vector<std::unique_ptr<RespType>>
  execute(const std::vector<std::unique_ptr<RespType>> &args,
          const AppConfig &config, unsigned socket_fd) {
    bool args_valid = validateArgs(args);
    if (!args_valid) {

      std::vector<std::unique_ptr<RespType>> result;
      result.push_back(std::make_unique<RespError>("Invalid args"));
      return result;
    }
    return executeImpl(args, config, socket_fd);
  }

  Type getType() const { return m_type; }

  bool isWriteCommand() const;

protected:
  Command(Type t) : m_type(t) {}

private:
  bool validateArgs(const std::vector<std::unique_ptr<RespType>> &args) {
    size_t nargs = args.size();
    for (int i = 0; i < nargs; ++i) {
      if (args[i]->getType() != RespType::BULK_STRING) {
        return false;
      }
    }
    return validateArgsImpl(args);
  }

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              const AppConfig &config, unsigned socket_fd) = 0;

  virtual bool
  validateArgsImpl(const std::vector<std::unique_ptr<RespType>> &args) = 0;

  Type m_type;
};

using CommandRegistry = Registry<std::string, Command>;

template <typename T>
using CommandRegistrar = Registrar<std::string, Command, T>;

class EchoCommand : public Command {
public:
  EchoCommand() : Command(ECHO) {}

private:
  static CommandRegistrar<EchoCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              const AppConfig &config, unsigned socket_fd) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 1;
  }
};

class PingCommand : public Command {
public:
  PingCommand() : Command(PING) {}

private:
  static CommandRegistrar<PingCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              const AppConfig &config, unsigned socket_fd) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 0;
  }
};

class SetCommand : public Command {
public:
  SetCommand() : Command(SET) {}

private:
  static CommandRegistrar<SetCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              const AppConfig &config, unsigned socket_fd) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 2 || args.size() == 4;
  }
};

class GetCommand : public Command {
public:
  GetCommand() : Command(GET) {}

private:
  static CommandRegistrar<GetCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              const AppConfig &config, unsigned socket_fd) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 1;
  }
};

class InfoCommand : public Command {
public:
  InfoCommand() : Command(INFO) {}

private:
  static CommandRegistrar<InfoCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              const AppConfig &config, unsigned socket_fd) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 1;
  }
};

class ReplConfCommand : public Command {
public:
  ReplConfCommand() : Command(REPLCONF) {}

private:
  static CommandRegistrar<ReplConfCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              const AppConfig &config, unsigned socket_fd) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() >= 2;
  }
};

class PsyncCommand : public Command {
public:
  PsyncCommand() : Command(PSYNC) {}

private:
  static CommandRegistrar<PsyncCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              const AppConfig &config, unsigned socket_fd) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 2;
  }
};

class WaitCommand : public Command {
public:
  WaitCommand() : Command(WAIT) {}

private:
  static CommandRegistrar<WaitCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              const AppConfig &config, unsigned socket_fd) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 2;
  }
};

class TypeCommand : public Command {
public:
  TypeCommand() : Command(TYPE) {}

private:
  static CommandRegistrar<TypeCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              const AppConfig &config, unsigned socket_fd) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 1;
  }
};
