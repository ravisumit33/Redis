#pragma once

#include "CommandFlags.hpp"
#include "RespType.hpp"
#include "connections/Capabilities.hpp"
#include <string_view>
#include <vector>

class AppConfig;

class ConfigCommand {
public:
  static constexpr std::string_view name = "CONFIG";
  static constexpr CmdFlags flags = CmdFlags::None;

  static bool validateArgs(const std::vector<RespValue> &args) {
    return args.size() == 2;
  }

  template <typename Ctx>
    requires Configurable<Ctx>
  static std::vector<RespValue> execute(const std::vector<RespValue> &args,
                                        Ctx &ctx) {
    return doExecute(args, ctx.config());
  }

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args,
                                          const AppConfig &config);
};
