#pragma once

#include <cstdint>

enum class CmdFlags : std::uint8_t {
  None = 0,
  Write = 1U << 0,
  Control = 1U << 1,
  SubscribedMode = 1U << 2,
};

constexpr CmdFlags operator|(CmdFlags lhs, CmdFlags rhs) {
  return static_cast<CmdFlags>(static_cast<std::uint8_t>(lhs) |
                               static_cast<std::uint8_t>(rhs));
}

constexpr bool hasFlag(CmdFlags set, CmdFlags flag) {
  return (static_cast<std::uint8_t>(set) & static_cast<std::uint8_t>(flag)) !=
         0;
}
