#include "Command.hpp"
#include <unordered_map>
#include <unordered_set>

bool Command::isWriteCommand() const {
  static const std::unordered_set<Type> write_commands = {SET};
  return write_commands.contains(getType());
}

bool Command::isControlCommand() const {
  static const std::unordered_set<Type> control_commands = {EXEC, DISCARD};
  return control_commands.contains(getType());
}

bool Command::isSubscribedModeCommand() const {
  static const std::unordered_set<Type> subscribed_mode_commands = {
      SUBSCRIBE, UNSUBSCRIBE, PING};
  return subscribed_mode_commands.contains(getType());
}

std::string Command::getTypeStr() const {
  static const std::unordered_map<Type, std::string> type_strings = {
      {ECHO, "ECHO"},
      {PING, "PING"},
      {SET, "SET"},
      {GET, "GET"},
      {INFO, "INFO"},
      {REPLCONF, "REPLCONF"},
      {PSYNC, "PSYNC"},
      {WAIT, "WAIT"},
      {TYPE, "TYPE"},
      {XADD, "XADD"},
      {XRANGE, "XRANGE"},
      {XREAD, "XREAD"},
      {INCR, "INCR"},
      {MULTI, "MULTI"},
      {EXEC, "EXEC"},
      {DISCARD, "DISCARD"},
      {CONFIG, "CONFIG"},
      {KEYS, "KEYS"},
      {RPUSH, "RPUSH"},
      {LPUSH, "LPUSH"},
      {LLEN, "LLEN"},
      {LPOP, "LPOP"},
      {LRANGE, "LRANGE"},
      {BLPOP, "BLPOP"},
      {SUBSCRIBE, "SUBSCRIBE"},
      {UNSUBSCRIBE, "UNSUBSCRIBE"},
      {PUBLISH, "PUBLISH"},
      {ZADD, "ZADD"},
      {ZRANK, "ZRANK"},
      {ZRANGE, "ZRANGE"},
  };

  auto it = type_strings.find(m_type);
  if (it != type_strings.end()) {
    return it->second;
  }
  throw std::logic_error("Unknown type of command");
}
