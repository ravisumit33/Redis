#pragma once

#include "RespType.hpp"
#include <string_view>
#include <vector>

class EchoCommand {
public:
  static constexpr std::string_view name = "ECHO";
  static constexpr bool is_write = false;
  static constexpr bool is_control = false;
  static constexpr bool is_subscribed_mode = false;

  static bool validateArgs(const std::vector<RespValue> &args) {
    return args.size() == 1;
  }

  template <typename Conn>
  static std::vector<RespValue> execute(const std::vector<RespValue> &args,
                                        Conn & /*conn*/) {
    return doExecute(args);
  }

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args);
};
