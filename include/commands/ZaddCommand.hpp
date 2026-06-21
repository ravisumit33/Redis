#pragma once

#include "CommandFlags.hpp"
#include "RespType.hpp"
#include "connections/Capabilities.hpp"
#include <string_view>
#include <vector>

class RedisStore;

class ZaddCommand {
public:
  static constexpr std::string_view name = "ZADD";
  static constexpr CmdFlags flags = CmdFlags::Write;

  static bool validateArgs(const std::vector<RespValue> &args) {
    return args.size() == 3;
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
