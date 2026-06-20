#pragma once

#include "RespType.hpp"
#include <string_view>
#include <vector>

class ClientConnection;

class UnsubscribeCommand {
public:
  static constexpr std::string_view name = "UNSUBSCRIBE";
  static constexpr bool is_write = false;
  static constexpr bool is_control = false;
  static constexpr bool is_subscribed_mode = true;

  static bool validateArgs(const std::vector<RespValue> &args) {
    return args.size() == 1;
  }

  static std::vector<RespValue> execute(const std::vector<RespValue> &args,
                                        ClientConnection &conn);
};
