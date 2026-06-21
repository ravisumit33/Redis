#pragma once

#include "RespType.hpp"
#include "connections/Capabilities.hpp"
#include <string_view>
#include <vector>

class TransactionState;

class MultiCommand {
public:
  static constexpr std::string_view name = "MULTI";
  static constexpr bool is_write = false;
  static constexpr bool is_control = true;
  static constexpr bool is_subscribed_mode = false;

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
