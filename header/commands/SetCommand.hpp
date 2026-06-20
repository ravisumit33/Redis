#pragma once

#include "RespType.hpp"
#include <string_view>
#include <vector>

class RedisStore;

class SetCommand {
public:
  static constexpr std::string_view name = "SET";
  static constexpr bool is_write = true;
  static constexpr bool is_control = false;
  static constexpr bool is_subscribed_mode = false;

  static bool validateArgs(const std::vector<RespValue> &args) {
    return args.size() == 2 || args.size() == 4;
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
