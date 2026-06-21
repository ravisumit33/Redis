#include "Command.hpp"
#include "AppContext.hpp"
#include "RespType.hpp"
#include "RespTypeParser.hpp"
#include "commands/BlpopCommand.hpp"
#include "commands/ConfigCommand.hpp"
#include "commands/DiscardCommand.hpp"
#include "commands/EchoCommand.hpp"
#include "commands/ExecCommand.hpp"
#include "commands/GeoaddCommand.hpp"
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
#include <istream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

bool isWriteCommand(const Command &cmd) {
  return std::visit([](const auto &command) { return command.is_write; }, cmd);
}

bool isControlCommand(const Command &cmd) {
  return std::visit([](const auto &command) { return command.is_control; },
                    cmd);
}

bool isSubscribedModeCommand(const Command &cmd) {
  return std::visit(
      [](const auto &command) { return command.is_subscribed_mode; }, cmd);
}

std::string_view getCommandName(const Command &cmd) {
  return std::visit(
      [](const auto &command) -> std::string_view { return command.name; },
      cmd);
}

void registerCommands(CommandRegistry &registry) {
  registry.add("BLPOP", [] { return Command{BlpopCommand{}}; });
  registry.add("CONFIG", [] { return Command{ConfigCommand{}}; });
  registry.add("DISCARD", [] { return Command{DiscardCommand{}}; });
  registry.add("ECHO", [] { return Command{EchoCommand{}}; });
  registry.add("EXEC", [] { return Command{ExecCommand{}}; });
  registry.add("GEOADD", [] { return Command{GeoaddCommand{}}; });
  registry.add("GET", [] { return Command{GetCommand{}}; });
  registry.add("INCR", [] { return Command{IncrCommand{}}; });
  registry.add("INFO", [] { return Command{InfoCommand{}}; });
  registry.add("KEYS", [] { return Command{KeysCommand{}}; });
  registry.add("LLEN", [] { return Command{LlenCommand{}}; });
  registry.add("LPOP", [] { return Command{LpopCommand{}}; });
  registry.add("LPUSH", [] { return Command{LpushCommand{}}; });
  registry.add("LRANGE", [] { return Command{LrangeCommand{}}; });
  registry.add("MULTI", [] { return Command{MultiCommand{}}; });
  registry.add("PING", [] { return Command{PingCommand{}}; });
  registry.add("PSYNC", [] { return Command{PsyncCommand{}}; });
  registry.add("PUBLISH", [] { return Command{PublishCommand{}}; });
  registry.add("REPLCONF", [] { return Command{ReplConfCommand{}}; });
  registry.add("RPUSH", [] { return Command{RpushCommand{}}; });
  registry.add("SET", [] { return Command{SetCommand{}}; });
  registry.add("SUBSCRIBE", [] { return Command{SubscribeCommand{}}; });
  registry.add("TYPE", [] { return Command{TypeCommand{}}; });
  registry.add("UNSUBSCRIBE", [] { return Command{UnsubscribeCommand{}}; });
  registry.add("WAIT", [] { return Command{WaitCommand{}}; });
  registry.add("XADD", [] { return Command{XaddCommand{}}; });
  registry.add("XRANGE", [] { return Command{XrangeCommand{}}; });
  registry.add("XREAD", [] { return Command{XreadCommand{}}; });
  registry.add("ZADD", [] { return Command{ZaddCommand{}}; });
  registry.add("ZCARD", [] { return Command{ZcardCommand{}}; });
  registry.add("ZRANGE", [] { return Command{ZrangeCommand{}}; });
  registry.add("ZRANK", [] { return Command{ZrankCommand{}}; });
  registry.add("ZREM", [] { return Command{ZremCommand{}}; });
  registry.add("ZSCORE", [] { return Command{ZscoreCommand{}}; });
}

std::pair<Command, std::vector<RespValue>> parseCommand(std::istream &in_stream,
                                                        AppContext &context) {
  char type{};
  in_stream.get(type);
  if (!in_stream) {
    throw std::runtime_error(
        "Command parsing: No RESP type found (empty stream)");
  }
  if (type != '*') {
    throw std::runtime_error(
        "Command parsing: Expected array type '*' but got '" +
        std::string(1, type) + "' (commands must be arrays)");
  }
  RespArray cmd_arr = parseRespArray(in_stream);
  auto command_args = cmd_arr.release();
  if (command_args.empty()) {
    throw std::runtime_error("Command parsing: Empty command array received");
  }
  auto &command_name_val = command_args.at(0);
  if (command_name_val.getType() != RespType::BULK_STRING) {
    throw std::runtime_error(
        "Command parsing: Command name must be bulk string, got type " +
        std::to_string(static_cast<int>(command_name_val.getType())));
  }
  auto cmd_name = getStringValue(command_name_val);
  auto cmd_opt = context.getCommandRegistry().get(cmd_name);
  if (!cmd_opt) {
    throw std::runtime_error("Command parsing: Unsupported command '" +
                             cmd_name + "'");
  }
  const Command command = *cmd_opt;
  command_args.erase(command_args.begin());
  return {command, std::move(command_args)};
}
