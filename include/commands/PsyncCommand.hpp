#pragma once

#include "CommandFlags.hpp"
#include "RespType.hpp"
#include "connections/Capabilities.hpp"
#include <string_view>
#include <vector>

class ReplicationManager;

class PsyncCommand {
public:
  static constexpr std::string_view name = "PSYNC";
  static constexpr CmdFlags flags = CmdFlags::None;

  static bool validateArgs(const std::vector<RespValue> &args) {
    return args.size() == 2;
  }

  template <typename Ctx>
    requires Replicating<Ctx>
  static std::vector<RespValue> execute(const std::vector<RespValue> &args,
                                        Ctx &ctx) {
    return doExecute(args, ctx.replication());
  }

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args,
                                          ReplicationManager &repl);
};
