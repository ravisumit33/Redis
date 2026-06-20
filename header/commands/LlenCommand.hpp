#pragma once

#include "RespType.hpp"
#include <string_view>
#include <vector>

class RedisStore;

class LlenCommand {
public:
  static constexpr std::string_view name = "LLEN";
  static constexpr bool is_write = false;
  static constexpr bool is_control = false;
  static constexpr bool is_subscribed_mode = false;

  static bool validateArgs(const std::vector<RespValue> &args) {
    return args.size() == 1;
  }

  template <typename Conn>
  std::vector<RespValue> execute(const std::vector<RespValue> &args,
                                 Conn &conn) const {
    return doExecute(args, conn.getContext().getRedisStore());
  }

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args,
                                          RedisStore &store);
};
