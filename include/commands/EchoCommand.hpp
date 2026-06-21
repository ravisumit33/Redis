#pragma once

#include "CommandFlags.hpp"
#include "RespType.hpp"
#include <string_view>
#include <vector>

class EchoCommand {
public:
  static constexpr std::string_view name = "ECHO";
  static constexpr CmdFlags flags = CmdFlags::None;

  static bool validateArgs(const std::vector<RespValue> &args) {
    return args.size() == 1;
  }

  template <typename Ctx>
  static std::vector<RespValue> execute(const std::vector<RespValue> &args,
                                        Ctx & /*ctx*/) {
    return doExecute(args);
  }

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args);
};
