#pragma once

#include "CommandFlags.hpp"
#include "RespType.hpp"
#include "connections/Capabilities.hpp"
#include <string_view>
#include <vector>

class AppConfig;
class ReplicationManager;

class InfoCommand {
public:
  static constexpr std::string_view name = "INFO";
  static constexpr CmdFlags flags = CmdFlags::None;

  static bool validateArgs(const std::vector<RespValue> &args) {
    return args.size() == 1;
  }

  template <typename Ctx>
    requires Configurable<Ctx> && Replicating<Ctx>
  static std::vector<RespValue> execute(const std::vector<RespValue> &args,
                                        Ctx &ctx) {
    return doExecute(args, ctx.config(), ctx.replication());
  }

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args,
                                          const AppConfig &config,
                                          ReplicationManager &repl);
};
