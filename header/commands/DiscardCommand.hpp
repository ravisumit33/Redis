#pragma once

#include "RespType.hpp"
#include <string_view>
#include <vector>

class ClientConnection;

class DiscardCommand {
public:
  static constexpr std::string_view name = "DISCARD";
  static constexpr bool is_write = false;
  static constexpr bool is_control = true;
  static constexpr bool is_subscribed_mode = false;

  static bool validateArgs(const std::vector<RespValue> &args) {
    return args.empty();
  }

  static std::vector<RespValue> execute(const std::vector<RespValue> &args,
                                        ClientConnection &conn);
};
