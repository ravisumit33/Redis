#pragma once

#include "Registrar.hpp"
#include "Registry.hpp"
#include "RespType.hpp"
#include <memory>
#include <vector>

class Connection;

class Command {
public:
  virtual ~Command() = default;

  enum Type {
    ECHO,
    PING,
    SET,
    GET,
    INFO,
    REPLCONF,
    PSYNC,
    WAIT,
    TYPE,
    XADD,
    XRANGE,
    XREAD,
    INCR,
    MULTI,
    EXEC,
    DISCARD,
    CONFIG,
    KEYS,
    RPUSH,
    LPUSH,
    LLEN,
    LPOP,
    LRANGE
  };

  std::vector<std::unique_ptr<RespType>>
  execute(const std::vector<std::unique_ptr<RespType>> &args,
          Connection &connection) {
    bool args_valid = validateArgs(args);
    if (!args_valid) {

      std::vector<std::unique_ptr<RespType>> result;
      result.push_back(std::make_unique<RespError>("Invalid args"));
      return result;
    }
    return executeImpl(args, connection);
  }

  Type getType() const { return m_type; }

  bool isWriteCommand() const;

  bool isControlCommand() const;

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
              Connection &connection) = 0;

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
              Connection &connection) override;

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
              Connection &connection) override;

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
              Connection &connection) override;

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
              Connection &connection) override;

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
              Connection &connection) override;

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
              Connection &connection) override;

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
              Connection &connection) override;

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
              Connection &connection) override;

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
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 1;
  }
};

class XaddCommand : public Command {
public:
  XaddCommand() : Command(XADD) {}

private:
  static CommandRegistrar<XaddCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    std::size_t nargs = args.size();
    return (nargs >= 4 && nargs % 2 == 0);
  }
};

class XrangeCommand : public Command {
public:
  XrangeCommand() : Command(XRANGE) {}

private:
  static CommandRegistrar<XrangeCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 3;
  }
};

class XreadCommand : public Command {
public:
  XreadCommand() : Command(XREAD) {}

private:
  static CommandRegistrar<XreadCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    std::size_t nargs = args.size();
    return (nargs >= 3) && (nargs % 2 == 1);
  }
};

class IncrCommand : public Command {
public:
  IncrCommand() : Command(INCR) {}

private:
  static CommandRegistrar<IncrCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 1;
  }
};

class MultiCommand : public Command {
public:
  MultiCommand() : Command(MULTI) {}

private:
  static CommandRegistrar<MultiCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 0;
  }
};

class ExecCommand : public Command {
public:
  ExecCommand() : Command(EXEC) {}

private:
  static CommandRegistrar<ExecCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 0;
  }
};

class DiscardCommand : public Command {
public:
  DiscardCommand() : Command(DISCARD) {}

private:
  static CommandRegistrar<DiscardCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 0;
  }
};

class ConfigCommand : public Command {
public:
  ConfigCommand() : Command(CONFIG) {}

private:
  static CommandRegistrar<ConfigCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 2;
  }
};

class KeysCommand : public Command {
public:
  KeysCommand() : Command(KEYS) {}

private:
  static CommandRegistrar<KeysCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 1;
  }
};

class RpushCommand : public Command {
public:
  RpushCommand() : Command(RPUSH) {}

private:
  static CommandRegistrar<RpushCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() >= 2;
  }
};

class LpushCommand : public Command {
public:
  LpushCommand() : Command(LPUSH) {}

private:
  static CommandRegistrar<LpushCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() >= 2;
  }
};

class LlenCommand : public Command {
public:
  LlenCommand() : Command(LLEN) {}

private:
  static CommandRegistrar<LlenCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 1;
  }
};

class LpopCommand : public Command {
public:
  LpopCommand() : Command(LPOP) {}

private:
  static CommandRegistrar<LpopCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 1 || args.size() == 2;
  }
};

class LrangeCommand : public Command {
public:
  LrangeCommand() : Command(LRANGE) {}

private:
  static CommandRegistrar<LrangeCommand> registrar;

  virtual std::vector<std::unique_ptr<RespType>>
  executeImpl(const std::vector<std::unique_ptr<RespType>> &args,
              Connection &connection) override;

  virtual bool validateArgsImpl(
      const std::vector<std::unique_ptr<RespType>> &args) override {
    return args.size() == 3;
  }
};
