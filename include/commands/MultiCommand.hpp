#pragma once

#include "CommandFlags.hpp"
#include "RespType.hpp"
#include "connections/Capabilities.hpp"
#include <string_view>
#include <vector>

class TransactionState;

class MultiCommand {
public:
  static constexpr std::string_view name = "MULTI";
  static constexpr CmdFlags flags = CmdFlags::Control;

  static bool validateArgs(const std::vector<RespValue> &args) {
    return args.empty();
  }

  template <typename Ctx>
    requires Transactional<Ctx>
  static std::vector<RespValue> execute(const std::vector<RespValue> &args,
                                        Ctx &ctx) {
    return doExecute(args, ctx.txn());
  }

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args,
                                          TransactionState &txn);
};
