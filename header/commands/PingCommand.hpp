#pragma once

#include "RespType.hpp"
#include "connections/Capabilities.hpp"
#include <string_view>
#include <vector>

class PingCommand {
public:
  static constexpr std::string_view name = "PING";
  static constexpr bool is_write = false;
  static constexpr bool is_control = false;
  static constexpr bool is_subscribed_mode = true;

  static bool validateArgs(const std::vector<RespValue> &args) {
    return args.empty();
  }

  template <typename Ctx>
  static std::vector<RespValue> execute(const std::vector<RespValue> & /*args*/,
                                        Ctx &ctx) {
    bool subscribed_mode = false;
    if constexpr (PubSub<Ctx>) {
      subscribed_mode = ctx.subs().inSubscribedMode();
    }
    return doExecute(subscribed_mode);
  }

private:
  static std::vector<RespValue> doExecute(bool subscribed_mode);
};
