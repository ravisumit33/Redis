#pragma once

#include "RespType.hpp"
#include "connections/Capabilities.hpp"
#include <string_view>
#include <vector>

class RedisStore;

class XreadCommand {
public:
  static constexpr std::string_view name = "XREAD";
  static constexpr bool is_write = false;
  static constexpr bool is_control = false;
  static constexpr bool is_subscribed_mode = false;

  static bool validateArgs(const std::vector<RespValue> &args) {
    const std::size_t nargs = args.size();
    return (nargs >= 3) && (nargs % 2 == 1);
  }

  template <typename Ctx>
    requires HasStore<Ctx>
  static std::vector<RespValue> execute(const std::vector<RespValue> &args,
                                        Ctx &ctx) {
    return doExecute(args, ctx.store());
  }

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args,
                                          RedisStore &store);
};
