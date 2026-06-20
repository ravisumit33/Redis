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
#include "connections/ClientConnection.hpp"
#include "connections/ServerConnection.hpp"
#include <istream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace {
bool validateCommandArgs(Command &cmd, const std::vector<RespValue> &args) {
  for (const auto &arg : args) {
    if (arg.getType() != RespType::BULK_STRING) {
      return false;
    }
  }
  return std::visit([&](auto &cmd) { return cmd.validateArgs(args); }, cmd);
}
} // namespace

std::vector<RespValue> executeCommand(Command &cmd,
                                      const std::vector<RespValue> &args,
                                      ClientConnection &conn) {
  if (!validateCommandArgs(cmd, args)) {
    return {RespError("Invalid args")};
  }
  return std::visit(
      [&](auto &cmd) -> std::vector<RespValue> {
        return cmd.execute(args, conn);
      },
      cmd);
}

std::vector<RespValue> executeCommand(Command &cmd,
                                      const std::vector<RespValue> &args,
                                      ServerConnection &conn) {
  if (!validateCommandArgs(cmd, args)) {
    return {RespError("Invalid args")};
  }
  return std::visit(
      [&](auto &cmd_var) -> std::vector<RespValue> {
        using T = std::decay_t<decltype(cmd_var)>;
        if constexpr (CommandFor<T, ServerConnection>) {
          return cmd_var.execute(args, conn);
        } else {
          throw std::logic_error(std::string("Command '") +
                                 std::string(T::name) +
                                 "' is not valid for server connection");
        }
      },
      cmd);
}

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
  registry["BLPOP"] = []() -> Command { return BlpopCommand{}; };
  registry["CONFIG"] = []() -> Command { return ConfigCommand{}; };
  registry["DISCARD"] = []() -> Command { return DiscardCommand{}; };
  registry["ECHO"] = []() -> Command { return EchoCommand{}; };
  registry["EXEC"] = []() -> Command { return ExecCommand{}; };
  registry["GEOADD"] = []() -> Command { return GeoaddCommand{}; };
  registry["GET"] = []() -> Command { return GetCommand{}; };
  registry["INCR"] = []() -> Command { return IncrCommand{}; };
  registry["INFO"] = []() -> Command { return InfoCommand{}; };
  registry["KEYS"] = []() -> Command { return KeysCommand{}; };
  registry["LLEN"] = []() -> Command { return LlenCommand{}; };
  registry["LPOP"] = []() -> Command { return LpopCommand{}; };
  registry["LPUSH"] = []() -> Command { return LpushCommand{}; };
  registry["LRANGE"] = []() -> Command { return LrangeCommand{}; };
  registry["MULTI"] = []() -> Command { return MultiCommand{}; };
  registry["PING"] = []() -> Command { return PingCommand{}; };
  registry["PSYNC"] = []() -> Command { return PsyncCommand{}; };
  registry["PUBLISH"] = []() -> Command { return PublishCommand{}; };
  registry["REPLCONF"] = []() -> Command { return ReplConfCommand{}; };
  registry["RPUSH"] = []() -> Command { return RpushCommand{}; };
  registry["SET"] = []() -> Command { return SetCommand{}; };
  registry["SUBSCRIBE"] = []() -> Command { return SubscribeCommand{}; };
  registry["TYPE"] = []() -> Command { return TypeCommand{}; };
  registry["UNSUBSCRIBE"] = []() -> Command { return UnsubscribeCommand{}; };
  registry["WAIT"] = []() -> Command { return WaitCommand{}; };
  registry["XADD"] = []() -> Command { return XaddCommand{}; };
  registry["XRANGE"] = []() -> Command { return XrangeCommand{}; };
  registry["XREAD"] = []() -> Command { return XreadCommand{}; };
  registry["ZADD"] = []() -> Command { return ZaddCommand{}; };
  registry["ZCARD"] = []() -> Command { return ZcardCommand{}; };
  registry["ZRANGE"] = []() -> Command { return ZrangeCommand{}; };
  registry["ZRANK"] = []() -> Command { return ZrankCommand{}; };
  registry["ZREM"] = []() -> Command { return ZremCommand{}; };
  registry["ZSCORE"] = []() -> Command { return ZscoreCommand{}; };
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
  auto itr = context.getCommandRegistry().find(cmd_name);
  if (itr == context.getCommandRegistry().end()) {
    throw std::runtime_error("Command parsing: Unsupported command '" +
                             cmd_name + "'");
  }
  const Command command = itr->second();
  command_args.erase(command_args.begin());
  return {command, std::move(command_args)};
}
