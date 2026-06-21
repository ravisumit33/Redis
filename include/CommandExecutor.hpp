#pragma once

#include "Command.hpp"
#include "RespType.hpp"
#include <concepts>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

template <typename Cmd, typename Ctx>
concept CommandFor =
    requires(const Cmd &cmd, const std::vector<RespValue> &args, Ctx &ctx) {
      { cmd.execute(args, ctx) } -> std::same_as<std::vector<RespValue>>;
    };

inline bool validateCommandArgs(Command &cmd,
                                const std::vector<RespValue> &args) {
  for (const auto &arg : args) {
    if (arg.getType() != RespType::BULK_STRING) {
      return false;
    }
  }
  return std::visit([&](auto &command) { return command.validateArgs(args); },
                    cmd);
}

template <typename Ctx>
std::vector<RespValue>
executeCommand(Command &cmd, const std::vector<RespValue> &args, Ctx &ctx) {
  if (!validateCommandArgs(cmd, args)) {
    return {RespError("Invalid args")};
  }
  return std::visit(
      [&](auto &command) -> std::vector<RespValue> {
        using T = std::remove_cvref_t<decltype(command)>;
        if constexpr (CommandFor<T, Ctx>) {
          return command.execute(args, ctx);
        } else {
          return {RespError("Command '" + std::string(T::name) +
                            "' is not valid for this connection")};
        }
      },
      cmd);
}
