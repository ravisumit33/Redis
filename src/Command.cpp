#include "Command.hpp"
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

class ClientConnection;
class ServerConnection;

#include "commands/BlpopCommand.hpp"
#include "commands/ConfigCommand.hpp"
#include "commands/DiscardCommand.hpp"
#include "commands/EchoCommand.hpp"
#include "commands/ExecCommand.hpp"
#include "commands/GetCommand.hpp"
#include "commands/IncrCommand.hpp"
#include "commands/InfoCommand.hpp"
#include "commands/KeysCommand.hpp"
#include "commands/LlenCommand.hpp"
#include "commands/LpopCommand.hpp"
#include "commands/LpushCommand.hpp"
#include "commands/LrangeCommand.hpp"
#include "commands/MultiCommand.hpp"
#include "commands/PingCommand.hpp"
#include "commands/PsyncCommand.hpp"
#include "commands/PublishCommand.hpp"
#include "commands/ReplConfCommand.hpp"
#include "commands/RpushCommand.hpp"
#include "commands/SetCommand.hpp"
#include "commands/SubscribeCommand.hpp"
#include "commands/TypeCommand.hpp"
#include "commands/UnsubscribeCommand.hpp"
#include "commands/WaitCommand.hpp"
#include "commands/XaddCommand.hpp"
#include "commands/XrangeCommand.hpp"
#include "commands/XreadCommand.hpp"
#include "commands/ZaddCommand.hpp"
#include "commands/ZcardCommand.hpp"
#include "commands/ZrangeCommand.hpp"
#include "commands/ZrankCommand.hpp"
#include "commands/ZremCommand.hpp"
#include "commands/ZscoreCommand.hpp"

bool Command::isWriteCommand() const {
  static const std::unordered_set<Type> write_commands = {Type::SET};
  return write_commands.contains(getType());
}

bool Command::isControlCommand() const {
  static const std::unordered_set<Type> control_commands = {Type::EXEC,
                                                            Type::DISCARD};
  return control_commands.contains(getType());
}

bool Command::isSubscribedModeCommand() const {
  static const std::unordered_set<Type> subscribed_mode_commands = {
      Type::SUBSCRIBE, Type::UNSUBSCRIBE, Type::PING};
  return subscribed_mode_commands.contains(getType());
}

std::string Command::getTypeStr(Type type) {
  static const std::unordered_map<Type, std::string> type_strings = {
      {Type::ECHO, "ECHO"},
      {Type::PING, "PING"},
      {Type::SET, "SET"},
      {Type::GET, "GET"},
      {Type::INFO, "INFO"},
      {Type::REPLCONF, "REPLCONF"},
      {Type::PSYNC, "PSYNC"},
      {Type::WAIT, "WAIT"},
      {Type::TYPE, "TYPE"},
      {Type::XADD, "XADD"},
      {Type::XRANGE, "XRANGE"},
      {Type::XREAD, "XREAD"},
      {Type::INCR, "INCR"},
      {Type::MULTI, "MULTI"},
      {Type::EXEC, "EXEC"},
      {Type::DISCARD, "DISCARD"},
      {Type::CONFIG, "CONFIG"},
      {Type::KEYS, "KEYS"},
      {Type::RPUSH, "RPUSH"},
      {Type::LPUSH, "LPUSH"},
      {Type::LLEN, "LLEN"},
      {Type::LPOP, "LPOP"},
      {Type::LRANGE, "LRANGE"},
      {Type::BLPOP, "BLPOP"},
      {Type::SUBSCRIBE, "SUBSCRIBE"},
      {Type::UNSUBSCRIBE, "UNSUBSCRIBE"},
      {Type::PUBLISH, "PUBLISH"},
      {Type::ZADD, "ZADD"},
      {Type::ZRANK, "ZRANK"},
      {Type::ZRANGE, "ZRANGE"},
      {Type::ZCARD, "ZCARD"},
      {Type::ZSCORE, "ZSCORE"},
      {Type::ZREM, "ZREM"},
  };

  auto itr = type_strings.find(type);
  if (itr != type_strings.end()) {
    return itr->second;
  }
  throw std::logic_error("Unknown type of command");
}

std::vector<RespValue>
Command::executeOnImpl(const std::vector<RespValue> & /*args*/,
                       ClientConnection & /*connection*/) {
  throw std::runtime_error("Command '" + getTypeStr(m_type) +
                           "' not supported for client connection");
}

std::vector<RespValue>
Command::executeOnImpl(const std::vector<RespValue> & /*args*/,
                       ServerConnection & /*connection*/) {
  throw std::runtime_error("Command '" + getTypeStr(m_type) +
                           "' not supported for server connection");
}

void registerCommands(CommandRegistry &registry) {
  registry.add(Command::getTypeStr(Command::Type::BLPOP),
               []() { return std::make_unique<BlpopCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::CONFIG),
               []() { return std::make_unique<ConfigCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::DISCARD),
               []() { return std::make_unique<DiscardCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::ECHO),
               []() { return std::make_unique<EchoCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::EXEC),
               []() { return std::make_unique<ExecCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::GET),
               []() { return std::make_unique<GetCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::INCR),
               []() { return std::make_unique<IncrCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::INFO),
               []() { return std::make_unique<InfoCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::KEYS),
               []() { return std::make_unique<KeysCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::LLEN),
               []() { return std::make_unique<LlenCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::LPOP),
               []() { return std::make_unique<LpopCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::LPUSH),
               []() { return std::make_unique<LpushCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::LRANGE),
               []() { return std::make_unique<LrangeCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::MULTI),
               []() { return std::make_unique<MultiCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::PING),
               []() { return std::make_unique<PingCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::PSYNC),
               []() { return std::make_unique<PsyncCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::PUBLISH),
               []() { return std::make_unique<PublishCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::REPLCONF),
               []() { return std::make_unique<ReplConfCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::RPUSH),
               []() { return std::make_unique<RpushCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::SET),
               []() { return std::make_unique<SetCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::SUBSCRIBE),
               []() { return std::make_unique<SubscribeCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::TYPE),
               []() { return std::make_unique<TypeCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::UNSUBSCRIBE),
               []() { return std::make_unique<UnsubscribeCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::WAIT),
               []() { return std::make_unique<WaitCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::XADD),
               []() { return std::make_unique<XaddCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::XRANGE),
               []() { return std::make_unique<XrangeCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::XREAD),
               []() { return std::make_unique<XreadCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::ZADD),
               []() { return std::make_unique<ZaddCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::ZCARD),
               []() { return std::make_unique<ZcardCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::ZRANGE),
               []() { return std::make_unique<ZrangeCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::ZRANK),
               []() { return std::make_unique<ZrankCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::ZREM),
               []() { return std::make_unique<ZremCommand>(); });
  registry.add(Command::getTypeStr(Command::Type::ZSCORE),
               []() { return std::make_unique<ZscoreCommand>(); });
}
