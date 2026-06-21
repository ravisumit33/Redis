#pragma once

#include "CommandFlags.hpp"
#include "RespType.hpp"
#include <string_view>
#include <vector>

class GeoaddCommand {
public:
  static constexpr std::string_view name = "GEOADD";
  static constexpr CmdFlags flags = CmdFlags::Write;

  static bool validateArgs(const std::vector<RespValue> &args) {
    return args.size() == 4;
  }

  template <typename Ctx>
  static std::vector<RespValue> execute(const std::vector<RespValue> &args,
                                        Ctx & /*ctx*/) {
    return doExecute(args);
  }

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args);
};
