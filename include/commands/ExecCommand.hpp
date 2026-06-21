#pragma once

#include "CommandFlags.hpp"
#include "RespType.hpp"
#include "connections/Capabilities.hpp"
#include <string_view>
#include <vector>

class ExecCommand {
public:
  static constexpr std::string_view name = "EXEC";
  static constexpr CmdFlags flags = CmdFlags::Control;

  static bool validateArgs(const std::vector<RespValue> &args) {
    return args.empty();
  }

  template <typename Ctx>
    requires Transactional<Ctx>
  static std::vector<RespValue> execute(const std::vector<RespValue> &args,
                                        Ctx &ctx);
};

template <typename Ctx>
  requires Transactional<Ctx>
std::vector<RespValue>
ExecCommand::execute(const std::vector<RespValue> & /*args*/, Ctx &ctx) {
  if (!ctx.txn().inTransaction()) {
    return {RespError("EXEC without MULTI")};
  }
  return ctx.txn().executeQueued(ctx);
}
