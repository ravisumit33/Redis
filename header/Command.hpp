#pragma once

#include "Registry.hpp"
#include "RespType.hpp"
#include <cstdint>
#include <string>
#include <vector>

class ClientConnection;
class ServerConnection;

class Command {
public:
  virtual ~Command() = default;
  Command(const Command &) = delete;
  Command &operator=(const Command &) = delete;
  Command(Command &&) = delete;
  Command &operator=(Command &&) = delete;

  enum class Type : std::uint8_t {
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
    PUBLISH,
    ZADD,
    ZRANK,
    ZRANGE,
    ZCARD,
    ZSCORE,
    ZREM,
    GEOADD,
  };

  std::vector<RespValue> execute(const std::vector<RespValue> &args,
                                 ClientConnection &connection) {
    bool args_valid = validateArgs(args);
    if (!args_valid) {
      std::vector<RespValue> result;
      result.emplace_back(RespError("Invalid args"));
      return result;
    }
    return executeOnImpl(args, connection);
  }

  std::vector<RespValue> execute(const std::vector<RespValue> &args,
                                 ServerConnection &connection) {
    bool args_valid = validateArgs(args);
    if (!args_valid) {
      std::vector<RespValue> result;
      result.emplace_back(RespError("Invalid args"));
      return result;
    }
    return executeOnImpl(args, connection);
  }

  Type getType() const { return m_type; }

  static std::string getTypeStr(Type type);

  bool isWriteCommand() const;

  bool isControlCommand() const;

  bool isSubscribedModeCommand() const;

protected:
  Command(Type cmd_type) : m_type(cmd_type) {}

  virtual std::vector<RespValue>
  executeOnImpl(const std::vector<RespValue> &args,
                ClientConnection &connection);

  virtual std::vector<RespValue>
  executeOnImpl(const std::vector<RespValue> &args,
                ServerConnection &connection);

private:
  bool validateArgs(const std::vector<RespValue> &args) {
    size_t nargs = args.size();
    for (size_t i = 0; i < nargs; ++i) {
      if (args[i].getType() != RespType::BULK_STRING) {
        return false;
      }
    }
    return validateArgsImpl(args);
  }

  virtual bool validateArgsImpl(const std::vector<RespValue> &args) = 0;

  Type m_type;
};

using CommandRegistry = Registry<std::string, Command>;

void registerCommands(CommandRegistry &registry);
