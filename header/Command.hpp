#pragma once

#include "Registrar.hpp"
#include "Registry.hpp"
#include "RespType.hpp"
#include <memory>
#include <string>
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
    LRANGE,
    BLPOP,
    SUBSCRIBE,
    UNSUBSCRIBE,
    PUBLISH
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

  std::string getTypeStr() const;

  bool isWriteCommand() const;

  bool isControlCommand() const;

  bool isSubscribedModeCommand() const;

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
