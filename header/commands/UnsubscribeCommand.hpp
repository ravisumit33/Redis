#pragma once

#include "CommandFlags.hpp"
#include "RespType.hpp"
#include "connections/Capabilities.hpp"
#include <string_view>
#include <vector>

class Subscriptions;

class UnsubscribeCommand {
public:
  static constexpr std::string_view name = "UNSUBSCRIBE";
  static constexpr CmdFlags flags = CmdFlags::SubscribedMode;

  static bool validateArgs(const std::vector<RespValue> &args) {
    return args.size() == 1;
  }

  template <typename Ctx>
    requires PubSub<Ctx>
  static std::vector<RespValue> execute(const std::vector<RespValue> &args,
                                        Ctx &ctx) {
    return doExecute(args, ctx.subs());
  }

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args,
                                          Subscriptions &subs);
};
